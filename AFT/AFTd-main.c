

#include <evd.h>

#include "AFT-node.h"

#define DEFAULT_HTTP_LISTEN_PORT  8080
#define DEFAULT_HTTPS_LISTEN_PORT 4430
#define DEFAULT_CONFIG_FILENAME "/etc/AFT/AFT.conf"

static EvdDaemon *evd_daemon;

static gchar *config_file = NULL;
static gboolean daemonize = FALSE;
static guint http_port = 0;
static guint https_port = 0;
static GKeyFile *config = NULL;

static AFTNode *http_node = NULL;
static AFTNode *https_node = NULL;

static gint setup_pending = 0;

static GOptionEntry entries[] =
{
  { "conf", 'c', 0, G_OPTION_ARG_STRING, &config_file, "Absolute path for the configuration file, default is '" DEFAULT_CONFIG_FILENAME "'", "filename" },
  { "daemonize", 'D', 0, G_OPTION_ARG_NONE, &daemonize, "Run service in the background", NULL },
  { "http-port", 'p', 0, G_OPTION_ARG_INT, &http_port, "Override the HTTP listening port specified in configuration file", "port" },
  { "https-port", 'P', 0, G_OPTION_ARG_INT, &https_port, "Override the HTTPS listening port specified in configuration file", "port" },
  { NULL }
};

static void
finish_setup (void)
{
  GError *error = NULL;
  gchar *user = NULL;

  /* set 'user' as process owner, if specified */
  user = g_key_file_get_string (config, "node", "user", NULL);
  if (user != NULL)
    {
      if (! evd_daemon_set_user (evd_daemon, user, &error))
        goto quit;

      g_free (user);
    }

  g_print ("Setup completed, AFT daemon running...\n");

  return;

 quit:
  g_print ("%s\n", error->message);
  g_error_free (error);

  evd_daemon_quit (evd_daemon, -1);
}

static void
node_on_listen (GObject      *service,
                GAsyncResult *result,
                gpointer      user_data)
{
  GError *error = NULL;
  guint *port = user_data;

  if (! evd_service_listen_finish (EVD_SERVICE (service), result, &error))
    goto quit;

  g_print ("Listening on port %u\n", *port);

  setup_pending--;
  if (setup_pending == 0)
    finish_setup ();

  return;

 quit:
  g_print ("ERROR listening on port %u: %s\n", *port, error->message);
  g_error_free (error);

  evd_daemon_quit (evd_daemon, -1);
}

static void
on_tls_certificate_loaded (GObject      *obj,
                           GAsyncResult *res,
                           gpointer      user_data)
{
  GError *error = NULL;
  EvdTlsCredentials *cred = EVD_TLS_CREDENTIALS (obj);

  if (! evd_tls_credentials_add_certificate_from_file_finish (cred,
                                                              res,
                                                              &error))
    {
      g_print ("ERROR loading TLS certificate: %s\n", error->message);
      g_error_free (error);

      evd_daemon_quit (evd_daemon, -1);
    }
  else
    {
      g_print ("TLS certificate loaded\n");

      setup_pending--;
      if (setup_pending == 0)
        finish_setup ();
    }
}

static gboolean
resolve_port (GKeyFile     *config,
              const gchar  *group_name,
              guint        *port,
              guint         default_port,
              GError      **error)
{
  if (*port == 0)
    {
      *port = g_key_file_get_integer (config, group_name, "port", error);
      if (*port == 0)
        {
          if ((*error) != NULL)
            {
              if (! g_error_matches (*error,
                                     G_KEY_FILE_ERROR,
                                     G_KEY_FILE_ERROR_KEY_NOT_FOUND))
                {
                  return FALSE;
                }
              else
                {
                  g_clear_error (error);
                }
            }
          else
            {
              g_set_error (error,
                           G_IO_ERROR,
                           G_IO_ERROR_INVALID_ARGUMENT,
                           "'0' is not a valid port");
              return FALSE;
            }
        }
    }

  if (*port == 0)
    *port = default_port;

  return TRUE;
}

static gboolean
setup_https_node (GKeyFile *config, GError **error)
{
  gchar *cert_file;
  gchar *key_file;
  EvdTlsCredentials *cred;
  gchar *addr;
  guint dh_depth;
  AFTWebService *web_service;

  /* load X.509 certificate filename from config */
  cert_file = g_key_file_get_string (config, "https", "cert", error);
  if (cert_file == NULL)
    return FALSE;

  /* load X.509 private-key filename from config */
  key_file = g_key_file_get_string (config, "https", "key", error);
  if (key_file == NULL)
    return FALSE;

  /* create AFT node for HTTPS */
  https_node = AFT_node_new (config, error);
  if (https_node == NULL)
    return FALSE;

  web_service = AFT_node_get_web_service (https_node);

  /* activate TLS automatically in the node */
  evd_service_set_tls_autostart (EVD_SERVICE (web_service), TRUE);

  /* obtain TLS credentials object from the node, and load the certificate
     asynchronously */
  cred = evd_service_get_tls_credentials (EVD_SERVICE (web_service));
  evd_tls_credentials_add_certificate_from_file (cred,
                                                 cert_file,
                                                 key_file,
                                                 NULL,
                                                 on_tls_certificate_loaded,
                                                 config);
  setup_pending++;

  /* set Diffie-Hellman parameters into credentials */
  dh_depth = g_key_file_get_integer (config, "https", "dh-depth", error);
  if (dh_depth == 0)
    {
      if ((*error) != NULL)
        {
          if (! g_error_matches (*error,
                                 G_KEY_FILE_ERROR,
                                 G_KEY_FILE_ERROR_KEY_NOT_FOUND))
            {
              return FALSE;
            }
          else
            {
              g_clear_error (error);
            }
        }
    }
  else
    {
      g_object_set (cred, "dh-bits", dh_depth, NULL);
    }

  /* obtain HTTPS listening port */
  if (! resolve_port (config,
                      "https",
                      &https_port,
                      DEFAULT_HTTPS_LISTEN_PORT,
                      error))
    {
      return FALSE;
    }

  /* start listening */
  addr = g_strdup_printf ("0.0.0.0:%d", https_port);
  evd_service_listen (EVD_SERVICE (web_service),
                      addr,
                      NULL,
                      node_on_listen,
                      &https_port);
  setup_pending++;
  g_free (addr);

  g_free (cert_file);
  g_free (key_file);

  return TRUE;
}

static gboolean
setup_http_node (GKeyFile *config, GError **error)
{
  gchar *addr;
  AFTWebService *web_service;

  /* create AFT node for HTTP */
  http_node = AFT_node_new (config, error);
  if (http_node == NULL)
    return FALSE;

  web_service = AFT_node_get_web_service (http_node);

  /* obtain HTTPS listening port */
  if (! resolve_port (config,
                      "http",
                      &http_port,
                      DEFAULT_HTTP_LISTEN_PORT,
                      error))
    {
      return FALSE;
    }

  /* start listening */
  addr = g_strdup_printf ("0.0.0.0:%d", http_port);
  evd_service_listen (EVD_SERVICE (web_service),
                      addr,
                      NULL,
                      node_on_listen,
                      &http_port);
  setup_pending++;
  g_free (addr);

  return TRUE;
}

gint
main (gint argc, gchar *argv[])
{
  gint exit_status = 0;
  GError *error = NULL;
  GOptionContext *context = NULL;
  gchar *pid_file;

  evd_tls_init (NULL);

  /* parse command line args */
  context = g_option_context_new ("- low friction file sharing service daemon");
  g_option_context_add_main_entries (context, entries, NULL);
  if (! g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("ERROR parsing commandline options: %s\n", error->message);
      g_error_free (error);

      exit_status = -1;
      goto out;
    }

  if (config_file == NULL)
    config_file = g_strdup (DEFAULT_CONFIG_FILENAME);

  /* load and parse configuration file */
  config = g_key_file_new ();
  if (! g_key_file_load_from_file (config,
                                   config_file,
                                   G_KEY_FILE_NONE,
                                   &error))
    {
      g_print ("ERROR loading configuration: %s\n", error->message);
      g_error_free (error);

      exit_status = -1;
      goto out;
    }

  /* main daemon */
  evd_daemon = evd_daemon_get_default (&argc, &argv);

  if (daemonize)
    evd_daemon_daemonize (evd_daemon, NULL);

  /* setup HTTPS service, if enabled in config file */
  if (g_key_file_get_boolean (config, "https", "enabled", NULL))
    {
      if (! setup_https_node (config, &error))
        {
          g_print ("ERROR setting up HTTPS node: %s\n", error->message);
          g_error_free (error);

          exit_status = -1;
          goto out;
        }
    }

  /* setup HTTP service, if enabled in config file */
  if (g_key_file_get_boolean (config, "http", "enabled", NULL))
    {
      if (! setup_http_node (config, &error))
        {
          g_print ("ERROR setting up HTTP node: %s\n", error->message);
          g_error_free (error);

          exit_status = -1;
          goto out;
        }
    }

  /* set PID file */
  pid_file = g_key_file_get_string (config, "node", "pid-file", NULL);
  if (pid_file != NULL && pid_file[0] != '\0')
    {
      evd_daemon_set_pid_file (evd_daemon, pid_file);
      g_free (pid_file);
    }

  /* start the show */
  if ((exit_status = evd_daemon_run (evd_daemon, &error)) < 0)
    {
      g_print ("ERROR running daemon: %s\n", error->message);
      g_error_free (error);
    }

  /* free stuff */
  if (http_node != NULL)
    g_object_unref (http_node);
  if (https_node != NULL)
    g_object_unref (https_node);

  g_object_unref (evd_daemon);

 out:
  g_option_context_free (context);
  g_free (config_file);
  if (config != NULL)
    g_key_file_free (config);

  evd_tls_deinit ();

  g_print ("\nAFT server terminated\n");

  return exit_status;
}
