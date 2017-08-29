#include "AFT-protocol.h"

typedef struct
{
  gchar *test_name;
  gint err_code;
  gchar *out_msg;
  gchar *in_msg;
} TestCase;

static TestCase test_cases[] =
  {
    {
      "register/ok",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123,"
      "    \"flags\": 7,"
      "    \"tags\": [\"trip\", \"outer\", \"space\"]"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":null,\"id\":\"1234abcd\",\"signature\":\"some secret signature\"}]}"
    },

    {
      "register/error/not-an-object",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [0]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Method register expects an array of objects\"}]}"
    },

    {
      "register/error/no-name",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source object expects a 'name' member to be a string\"}]}"
    },

    {
      "register/error/no-type",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"\""
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source object expects a 'type' member to be a string\"}]}"
    },

    {
      "register/error/no-size",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"flags\": 7"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":null,\"id\":\"1234abcd\",\"signature\":\"some secret signature\"}]}"
    },

    {
      "register/error/no-flags",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source object expects a 'flags' member to be a number\"}]}"
    },

    {
      "register/ok/no-tags",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123,"
      "    \"flags\": 7"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":null,\"id\":\"1234abcd\",\"signature\":\"some secret signature\"}]}"
    },

    {
      "register/error/size-is-negative",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": -123"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source size must be equal or greater than zero\"}]}"
    },

    {
      "register/error/flags-is-negative",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123,"
      "    \"flags\": -1"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source flags must be equal or greater than zero\"}]}"
    },

    {
      "register/ok/multiple",
      0,
      "{"
      "  \"method\": \"register\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123,"
      "    \"flags\": 7,"
      "    \"tags\": [\"trip\", \"outer\", \"space\"]"
      "  },"
      "  {"
      "    \"name\": \"Some content\","
      "    \"type\": \"text/plain\","
      "    \"size\": 123,"
      "    \"flags\": 7,"
      "    \"tags\": [\"trip\", \"outer\", \"space\"]"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":null,\"id\":\"1234abcd\",\"signature\":\"some secret signature\"},{\"error\":null,\"id\":\"1234abcd\",\"signature\":\"some secret signature\"}]}"
    },

    {
      "unregister/ok",
      0,
      "{"
      "  \"method\": \"unregister\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "      \"id\": \"abcd1234\""
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"result\":true}]}"
    },

    {
      "unregister/error/id-not-string",
      0,
      "{"
      "  \"method\": \"unregister\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "      \"id\": 12345678"
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source id must be a valid string\"}]}"
    },

    {
      "unregister/error/id-is-empty",
      0,
      "{"
      "  \"method\": \"unregister\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "      \"id\": \"\""
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"error\":\"Source id must be a valid string\"}]}"
    },

    {
      "unregister/ok/multiple",
      0,
      "{"
      "  \"method\": \"unregister\","
      "  \"id\": 5,"
      "  \"params\": [ {"
      "      \"id\": \"abcd1234\""
      "  }, {"
      "      \"id\": \"abcd1234\""
      "  } ]"
      "}",
      "{\"id\":5,\"error\":null,\"result\":[{\"result\":true},{\"result\":true}]}"
    },

  };

typedef struct
{
  AFTProtocolVTable vtable;
  AFTProtocol *protocol;
  EvdPeer *peer;
} Fixture;

static gboolean        register_source             (AFTProtocol  *protocol,
                                                    EvdPeer          *peer,
                                                    AFTSource    *source,
                                                    GError          **error,
                                                    gpointer          user_data);
static gboolean        unregister_source           (AFTProtocol *protocol,
                                                    EvdPeer         *peer,
                                                    const gchar     *id,
                                                    gboolean         gracefully,
                                                    gpointer         user_data);

static void
fixture_setup (Fixture       *f,
               gconstpointer  data)
{
  EvdWebTransportServer *transport;

  f->vtable.register_source = register_source;
  f->vtable.unregister_source = unregister_source;
  f->protocol = AFT_protocol_new (&f->vtable, f, NULL);

  transport = evd_web_transport_server_new (NULL);
  f->peer = g_object_new (EVD_TYPE_PEER,
                          "transport", transport,
                          NULL);
  g_object_unref (transport);
}

static void
fixture_teardown (Fixture       *f,
                  gconstpointer  data)
{
  g_assert (G_OBJECT (f->protocol)->ref_count == 1);
  g_object_unref (f->protocol);
  g_object_unref (f->peer);
}

static void
test_new (Fixture       *f,
          gconstpointer  data)
{
  g_assert (AFT_IS_PROTOCOL (f->protocol));
  g_assert (EVD_IS_JSONRPC (AFT_protocol_get_rpc (f->protocol)));
}

static gboolean
register_source (AFTProtocol  *protocol,
                 EvdPeer          *peer,
                 AFTSource    *source,
                 GError          **error,
                 gpointer          user_data)
{
  Fixture *f = user_data;
  const gchar **tags;

  g_assert (AFT_IS_PROTOCOL (protocol));
  g_assert (protocol == f->protocol);

  g_assert (AFT_IS_SOURCE (source));

  g_assert (EVD_IS_PEER (peer));
  g_assert (peer == f->peer);

  g_assert_cmpstr (AFT_source_get_name (source), ==, "Some content");
  g_assert_cmpstr (AFT_source_get_content_type (source), ==, "text/plain");
  if (AFT_source_get_size (source) > 0)
    g_assert_cmpuint (AFT_source_get_size (source), ==, 123);
  g_assert_cmpuint (AFT_source_get_flags (source), ==, 7);

  tags = AFT_source_get_tags (source);

  if (tags != NULL)
    {
      g_assert_cmpstr (tags[0], ==, "trip");
      g_assert_cmpstr (tags[1], ==, "outer");
      g_assert_cmpstr (tags[2], ==, "space");
      g_assert_cmpstr (tags[3], ==, NULL);
    }

  AFT_source_set_id (source, "1234abcd");
  AFT_source_set_signature (source, "some secret signature");

  return TRUE;
}

static gboolean
unregister_source (AFTProtocol *protocol,
                   EvdPeer         *peer,
                   const gchar     *id,
                   gboolean         gracefully,
                   gpointer         user_data)
{
  Fixture *f = user_data;

  g_assert (AFT_IS_PROTOCOL (protocol));
  g_assert (protocol == f->protocol);

  g_assert (EVD_IS_PEER (peer));
  g_assert (peer == f->peer);

  g_assert_cmpstr (id, ==, "abcd1234");

  return TRUE;
}

static void
test_func (Fixture       *f,
           gconstpointer  data)
{
  EvdJsonrpc *rpc;
  GError *error = NULL;
  gchar *msg;
  gsize size;
  TestCase *test_case = (TestCase *) data;

  rpc = AFT_protocol_get_rpc (f->protocol);

  evd_jsonrpc_transport_receive (rpc, test_case->out_msg, f->peer, 1, &error);
  g_assert_no_error (error);

  if (test_case->in_msg != NULL)
    {
      msg = evd_peer_pop_message (f->peer, &size, NULL);
      g_assert_cmpstr (msg, ==, test_case->in_msg);

      g_free (msg);
    }
}

gint
main (gint argc, gchar *argv[])
{
  gint i;

#ifndef GLIB_VERSION_2_36
  g_type_init ();
#endif

  g_test_init (&argc, &argv, NULL);

  g_test_add ("/protocol/new",
              Fixture,
              NULL,
              fixture_setup,
              test_new,
              fixture_teardown);

  for (i=0; i<sizeof (test_cases) / sizeof (TestCase); i++)
    {
      gchar *test_path;

      test_path = g_strdup_printf ("/protocol/%s", test_cases[i].test_name);

      g_test_add (test_path,
                  Fixture,
                  &test_cases[i],
                  fixture_setup,
                  test_func,
                  fixture_teardown);

      g_free (test_path);
    }

  return g_test_run ();
}
