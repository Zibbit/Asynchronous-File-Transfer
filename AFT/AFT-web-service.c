

#include "AFT-web-service.h"

G_DEFINE_TYPE (AFTWebService, AFT_web_service, EVD_TYPE_WEB_SERVICE)

#define AFT_WEB_SERVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                              AFT_TYPE_WEB_SERVICE, \
                                              AFTWebServicePrivate))

#define PATH_ACTION_VIEW "view"

#define API_PATH        "api"
#define SCRIPT_PATH     "js"
#define MANAGEMENT_PATH "mgmt"
#define TRANSPORT_PATH  "transport"

/* private data */
struct _AFTWebServicePrivate
{
  EvdWebTransportServer *transport;
  EvdWebSelector *selector;
  EvdWebDir *webdir;

  gchar *server_name;
  gboolean force_https;
  guint https_port;

  gchar *log_filename;
  GFileOutputStream *log_output_stream;
  GQueue *log_queue;

  AFTWebServiceContentRequestCb content_req_cb;
  gpointer user_data;
};

static void     AFT_web_service_class_init         (AFTWebServiceClass *class);
static void     AFT_web_service_init               (AFTWebService *self);

static void     AFT_web_service_finalize           (GObject *obj);
static void     AFT_web_service_dispose            (GObject *obj);

static void     request_handler                        (EvdWebService     *self,
                                                        EvdHttpConnection *conn,
                                                        EvdHttpRequest    *request);

static void     web_dir_on_log_entry                   (EvdWebService *service,
                                                        const gchar   *entry,
                                                        gpointer       user_data);

static void
AFT_web_service_class_init (AFTWebServiceClass *class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (class);
  EvdWebServiceClass *ws_class = EVD_WEB_SERVICE_CLASS (class);

  obj_class->dispose = AFT_web_service_dispose;
  obj_class->finalize = AFT_web_service_finalize;

  ws_class->request_handler = request_handler;

  g_type_class_add_private (obj_class, sizeof (AFTWebServicePrivate));
}

static void
AFT_web_service_init (AFTWebService *self)
{
  AFTWebServicePrivate *priv;
  EvdWebService *ws, *lp;

  priv = AFT_WEB_SERVICE_GET_PRIVATE (self);
  self->priv = priv;

  evd_web_service_set_origin_policy (EVD_WEB_SERVICE (self),
                                     EVD_POLICY_ALLOW);

  /* web transport */
  priv->transport = evd_web_transport_server_new ("/" TRANSPORT_PATH);
  evd_web_service_set_origin_policy (EVD_WEB_SERVICE (priv->transport),
                                     EVD_POLICY_ALLOW);

  g_object_get (priv->transport,
                "lp-service", &lp,
                "websocket-service", &ws,
                NULL);

  evd_websocket_server_set_standalone (EVD_WEBSOCKET_SERVER (ws), TRUE);

  g_object_unref (lp);
  g_object_unref (ws);

  evd_web_service_set_origin_policy (lp, EVD_POLICY_ALLOW);
  evd_web_service_set_origin_policy (ws, EVD_POLICY_ALLOW);

  /* web dir */
  priv->webdir = evd_web_dir_new ();
  evd_web_dir_set_root (priv->webdir, HTML_DATA_DIR);
  evd_web_service_set_origin_policy (EVD_WEB_SERVICE (priv->webdir),
                                     EVD_POLICY_ALLOW);

  /* web selector */
  priv->selector = evd_web_selector_new ();

  evd_web_transport_server_use_selector (priv->transport, priv->selector);
  evd_web_selector_set_default_service (priv->selector,
                                        EVD_SERVICE (priv->webdir));
  evd_web_service_set_origin_policy (EVD_WEB_SERVICE (priv->selector),
                                     EVD_POLICY_ALLOW);


  priv->server_name = NULL;
  priv->force_https = FALSE;
  priv->https_port = 0;

  priv->log_filename = NULL;
  priv->log_output_stream = NULL;
  priv->log_queue = NULL;
}

static void
AFT_web_service_dispose (GObject *obj)
{
  AFTWebService *self = AFT_WEB_SERVICE (obj);

  if (self->priv->transport != NULL)
    {
      g_object_unref (self->priv->transport);
      self->priv->transport = NULL;
    }

  G_OBJECT_CLASS (AFT_web_service_parent_class)->dispose (obj);
}

static void
AFT_web_service_finalize (GObject *obj)
{
  AFTWebService *self = AFT_WEB_SERVICE (obj);

  g_free (self->priv->server_name);

  g_object_unref (self->priv->webdir);
  g_object_unref (self->priv->selector);

  if (self->priv->log_queue != NULL)
    {
      while (g_queue_get_length (self->priv->log_queue) > 0)
        {
          gchar *entry;
          entry = g_queue_pop_head (self->priv->log_queue);
          g_free (entry);
        }
      g_queue_free (self->priv->log_queue);
    }

  if (self->priv->log_output_stream != NULL)
    g_object_unref (self->priv->log_output_stream);

  if (self->priv->log_filename != NULL)
    g_free (self->priv->log_filename);

  G_OBJECT_CLASS (AFT_web_service_parent_class)->finalize (obj);
}

static void
request_handler (EvdWebService     *web_service,
                 EvdHttpConnection *conn,
                 EvdHttpRequest    *request)
{
  AFTWebService *self = AFT_WEB_SERVICE (web_service);
  gchar **tokens;
  SoupURI *uri;

  uri = evd_http_request_get_uri (request);

  if ( (self->priv->force_https &&
        ! evd_connection_get_tls_active (EVD_CONNECTION (conn))) ||
       (self->priv->server_name != NULL &&
        g_strcmp0 (soup_uri_get_host (uri), self->priv->server_name) != 0) )
    {
      gchar *new_uri;

      if (self->priv->force_https)
        {
          if (g_strcmp0 (soup_uri_get_scheme (uri), "ws") == 0)
            soup_uri_set_scheme (uri, "wss");
          else
            soup_uri_set_scheme (uri, "https");
          soup_uri_set_port (uri, self->priv->https_port);
        }

      soup_uri_set_host (uri, self->priv->server_name);

      new_uri = soup_uri_to_string (uri, FALSE);

      evd_http_connection_redirect (conn, new_uri, FALSE, NULL);

      g_free (new_uri);

      return;
    }

  tokens = g_strsplit (uri->path, "/", 16);

  /* request to a JS script or to the main transport */
  if (g_strcmp0 (tokens[1], SCRIPT_PATH) == 0 ||
      g_strcmp0 (tokens[1], TRANSPORT_PATH) == 0)
    {
      /* forward it to the Web selector */
      evd_web_service_add_connection_with_request
        (EVD_WEB_SERVICE (self->priv->selector),
         conn,
         request,
         EVD_SERVICE (self));
    }
  /* request to the RESTful API */
  else if (g_strcmp0 (tokens[1], API_PATH) == 0)
    {
      /* @TODO */
    }
  /* request to the management API */
  else if (g_strcmp0 (tokens[1], MANAGEMENT_PATH) == 0)
    {
      /* @TODO */
    }
  else
    {
      /* if none of the above, assume it is a content related request */
      self->priv->content_req_cb (self,
                                  tokens[1],
                                  conn,
                                  request,
                                  self->priv->user_data);
    }

  g_strfreev (tokens);
}

static void
set_max_node_bandwidth (AFTWebService *self,
                        gdouble            max_bw_in,
                        gdouble            max_bw_out)
{
  EvdStreamThrottle *throttle;

  g_object_get (self,
                "input-throttle", &throttle,
                NULL);
  g_object_set (throttle,
                "bandwidth", max_bw_in,
                NULL);
  g_object_unref (throttle);

  g_object_get (self,
                "output-throttle", &throttle,
                NULL);
  g_object_set (throttle,
                "bandwidth", max_bw_out,
                NULL);
  g_object_unref (throttle);
}

static void
web_dir_on_log_entry_written (GObject      *obj,
                              GAsyncResult *res,
                              gpointer      user_data)
{
  AFTWebService *self = AFT_WEB_SERVICE (user_data);
  gchar *entry = user_data;
  GError *error = NULL;

  g_output_stream_write_finish (G_OUTPUT_STREAM (obj), res, &error);
  if (error != NULL)
    {
      g_warning ("Failed to write to log file: %s", error->message);
      g_error_free (error);
    }

  entry = g_queue_pop_head (self->priv->log_queue);
  g_free (entry);

  if (g_queue_get_length (self->priv->log_queue) > 0)
    {
      entry = g_queue_peek_head (self->priv->log_queue);
      g_output_stream_write_async (G_OUTPUT_STREAM (self->priv->log_output_stream),
                                   entry,
                                   strlen (entry),
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   web_dir_on_log_entry_written,
                                   self);
    }
}

static void
web_dir_on_log_entry (EvdWebService *service,
                      const gchar   *entry,
                      gpointer       user_data)
{
  AFTWebService *self = AFT_WEB_SERVICE (user_data);
  gchar *copy;

  copy = g_strdup_printf ("%s\n", entry);

  g_queue_push_tail (self->priv->log_queue, copy);

  if (! g_output_stream_has_pending (G_OUTPUT_STREAM (self->priv->log_output_stream)))
    g_output_stream_write_async (G_OUTPUT_STREAM (self->priv->log_output_stream),
                                 copy,
                                 strlen (copy),
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 web_dir_on_log_entry_written,
                                 self);
}

static void
setup_web_dir_logging (AFTWebService *self)
{
  GError *error = NULL;
  GFile *log_file;

  log_file = g_file_new_for_path (self->priv->log_filename);

  self->priv->log_output_stream = g_file_append_to (log_file,
                                                    G_FILE_CREATE_NONE,
                                                    NULL,
                                                    &error);

  if (self->priv->log_output_stream == NULL)
    {
      g_warning ("Failed opening log file: %s. (HTTP logs disabled)",
                 error->message);
      g_error_free (error);

      return;
    }

  self->priv->log_queue = g_queue_new ();

  g_signal_connect (self->priv->webdir,
                    "log-entry",
                    G_CALLBACK (web_dir_on_log_entry),
                    self);

  g_object_unref (log_file);
}

static gboolean
load_config (AFTWebService *self, GKeyFile *config, GError **error)
{
  /* global bandwidth limites */
  set_max_node_bandwidth (self,
                          g_key_file_get_double (config,
                                                 "node",
                                                 "max-bandwidth-in",
                                                 NULL),
                          g_key_file_get_double (config,
                                                 "node",
                                                 "max-bandwidth-out",
                                                 NULL));

  /* force https? */
  self->priv->force_https = g_key_file_get_boolean (config,
                                                    "http",
                                                    "force-https",
                                                    NULL) == TRUE;
  if (self->priv->force_https)
    {
      self->priv->https_port = g_key_file_get_integer (config,
                                                       "https",
                                                       "port",
                                                       NULL);
    }

  /* log file */
  self->priv->log_filename = g_key_file_get_string (config,
                                                    "log",
                                                    "http-log-file",
                                                    NULL);
  if (self->priv->log_filename != NULL && self->priv->log_filename[0] != '\0')
    setup_web_dir_logging (self);

  /* server name */
  self->priv->server_name = g_key_file_get_string (config,
                                                   "node",
                                                   "server-name",
                                                    NULL);
  if (self->priv->server_name != NULL && self->priv->server_name[0] == '\0')
    {
      g_free (self->priv->server_name);
      self->priv->server_name = NULL;
    }

  return TRUE;
}

/* public methods */

AFTWebService *
AFT_web_service_new (GKeyFile                           *config,
                         AFTWebServiceContentRequestCb   content_req_cb,
                         gpointer                            user_data,
                         GError                            **error)
{
  AFTWebService *self;

  g_return_val_if_fail (config != NULL, NULL);
  g_return_val_if_fail (content_req_cb != NULL, NULL);

  self = g_object_new (AFT_TYPE_WEB_SERVICE, NULL);

  if (! load_config (self, config, error))
    goto err;

  self->priv->content_req_cb = content_req_cb;
  self->priv->user_data = user_data;

  return self;

 err:
  g_object_unref (self);
  return NULL;
}

EvdTransport *
AFT_web_service_get_transport (AFTWebService *self)
{
  g_return_val_if_fail (AFT_IS_WEB_SERVICE (self), NULL);

  return EVD_TRANSPORT (self->priv->transport);
}

#ifdef ENABLE_TESTS

#endif /* ENABLE_TESTS */
