

#include "AFT-transfer.h"

G_DEFINE_TYPE (AFTTransfer, AFT_transfer, G_TYPE_OBJECT)

#define AFT_TRANSFER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                           AFT_TYPE_TRANSFER, \
                                           AFTTransferPrivate))

#define BLOCK_SIZE 0x4000

#define START_TIMEOUT 30000 /* in miliseconds */

/* private data */
struct _AFTTransferPrivate
{
  gchar *id;
  guint status;

  EvdWebService *web_service;
  AFTSource *source;
  EvdHttpConnection *source_conn;
  EvdHttpConnection *target_conn;
  EvdPeer *target_peer;
  GCancellable *cancellable;

  gchar *action;
  gboolean is_chunked;
  SoupRange byte_range;

  gchar *buf;
  gsize transfer_len;
  gsize transferred;
  gdouble bandwidth;

  GSimpleAsyncResult *result;

  gboolean download;

  guint timeout_src_id;
};

static void     AFT_transfer_class_init         (AFTTransferClass *class);
static void     AFT_transfer_init               (AFTTransfer *self);

static void     AFT_transfer_finalize           (GObject *obj);
static void     AFT_transfer_dispose            (GObject *obj);


static void     target_connection_on_close          (EvdHttpConnection *conn,
                                                     gpointer           user_data);
static void     source_connection_on_close          (EvdHttpConnection *conn,
                                                     gpointer           user_data);

static void     AFT_transfer_read               (AFTTransfer *self);
static void     AFT_transfer_flush_target       (AFTTransfer *self);
static void     AFT_transfer_complete           (AFTTransfer *self);

static void
AFT_transfer_class_init (AFTTransferClass *class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (class);

  obj_class->dispose = AFT_transfer_dispose;
  obj_class->finalize = AFT_transfer_finalize;

  g_type_class_add_private (obj_class, sizeof (AFTTransferPrivate));
}

static void
AFT_transfer_init (AFTTransfer *self)
{
  AFTTransferPrivate *priv;

  priv = AFT_TRANSFER_GET_PRIVATE (self);
  self->priv = priv;

  priv->timeout_src_id = 0;
}

static void
AFT_transfer_dispose (GObject *obj)
{
  AFTTransfer *self = AFT_TRANSFER (obj);

  if (self->priv->source != NULL)
    {
      g_object_unref (self->priv->source);
      self->priv->source = NULL;
    }

  if (self->priv->target_conn != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->priv->target_conn,
                                            target_connection_on_close,
                                            self);
      g_object_unref (self->priv->target_conn);
      self->priv->target_conn = NULL;
    }

  if (self->priv->source_conn != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->priv->source_conn,
                                            source_connection_on_close,
                                            self);
      g_object_unref (self->priv->source_conn);
      self->priv->source_conn = NULL;
    }

  if (self->priv->target_peer != NULL)
    {
      g_object_unref (self->priv->target_peer);
      self->priv->target_peer = NULL;
    }

  G_OBJECT_CLASS (AFT_transfer_parent_class)->dispose (obj);
}

static void
AFT_transfer_finalize (GObject *obj)
{
  AFTTransfer *self = AFT_TRANSFER (obj);

  g_free (self->priv->id);
  g_free (self->priv->action);

  if (self->priv->buf != NULL)
    g_slice_free1 (BLOCK_SIZE, self->priv->buf);

  if (self->priv->cancellable != NULL)
    g_object_unref (self->priv->cancellable);

  if (self->priv->timeout_src_id != 0)
    {
      g_source_remove (self->priv->timeout_src_id);
      self->priv->timeout_src_id = 0;
    }

  g_object_unref (self->priv->web_service);

  G_OBJECT_CLASS (AFT_transfer_parent_class)->finalize (obj);
}

static void
target_connection_on_close (EvdHttpConnection *conn, gpointer user_data)
{
  AFTTransfer *self = user_data;

  g_simple_async_result_set_error (self->priv->result,
                                   G_IO_ERROR,
                                   G_IO_ERROR_CLOSED,
                                   "Target connection dropped");

  self->priv->status = AFT_TRANSFER_STATUS_TARGET_ABORTED;

  AFT_transfer_complete (self);
}

static void
source_connection_on_close (EvdHttpConnection *conn, gpointer user_data)
{
  AFTTransfer *self = user_data;

  g_simple_async_result_set_error (self->priv->result,
                                   G_IO_ERROR,
                                   G_IO_ERROR_CLOSED,
                                   "Source connection dropped");

  self->priv->status = AFT_TRANSFER_STATUS_SOURCE_ABORTED;

  AFT_transfer_complete (self);
}

static void
AFT_transfer_on_target_can_write (EvdConnection *target_conn,
                                      gpointer       user_data)
{
  AFTTransfer *self = user_data;

  evd_connection_unlock_close (EVD_CONNECTION (self->priv->source_conn));
  AFT_transfer_read (self);
}

static void
AFT_transfer_on_read (GObject      *obj,
                          GAsyncResult *res,
                          gpointer      user_data)
{
  AFTTransfer *self = user_data;
  gssize size;
  GError *error = NULL;
  EvdStreamThrottle *throttle;

  size = g_input_stream_read_finish (G_INPUT_STREAM (obj), res, &error);
  if (size < 0)
    {
      g_printerr ("ERROR reading from source: %s\n", error->message);

      g_simple_async_result_take_error (self->priv->result, error);
      self->priv->status = AFT_TRANSFER_STATUS_ERROR;
      AFT_transfer_complete (self);
      goto out;
    }

  if (! evd_http_connection_write_content (self->priv->target_conn,
                                           self->priv->buf,
                                           size,
                                           TRUE,
                                           &error))
    {
      g_printerr ("ERROR writing to target: %s\n", error->message);

      g_simple_async_result_take_error (self->priv->result, error);
      self->priv->status = AFT_TRANSFER_STATUS_ERROR;
      AFT_transfer_complete (self);

      goto out;
    }

  self->priv->transferred += size;

  throttle =
    evd_io_stream_get_input_throttle (EVD_IO_STREAM (self->priv->target_conn));
  self->priv->bandwidth = evd_stream_throttle_get_actual_bandwidth (throttle);

  g_assert (self->priv->transferred <= self->priv->transfer_len);
  if (self->priv->transferred == self->priv->transfer_len)
    {
      /* finished reading, send HTTP response to source */
      g_signal_handlers_disconnect_by_func (self->priv->source_conn,
                                            source_connection_on_close,
                                            self);

      if (! evd_web_service_respond (self->priv->web_service,
                                     self->priv->source_conn,
                                     SOUP_STATUS_OK,
                                     NULL,
                                     NULL,
                                     0,
                                     &error))
        {
          g_printerr ("Error sending response to source: %s\n", error->message);
          g_error_free (error);
          self->priv->status = AFT_TRANSFER_STATUS_ERROR;
        }

      AFT_transfer_flush_target (self);
    }
  else
    {
      AFT_transfer_read (self);
    }

 out:
  g_object_unref (self);
}

static void
AFT_transfer_read (AFTTransfer *self)
{
  GInputStream *stream;

  stream = g_io_stream_get_input_stream (G_IO_STREAM (self->priv->source_conn));

  if (g_input_stream_has_pending (stream))
    {
      g_warning ("has pending\n");
      return;
    }

  if (evd_connection_get_max_writable (EVD_CONNECTION (self->priv->target_conn)) > 0)
    {
      gssize size;

      size = MIN (BLOCK_SIZE,
                  AFT_source_get_size (self->priv->source) - self->priv->transferred);
      if (size <= 0)
        return;

      g_object_ref (self);
      g_input_stream_read_async (stream,
                                 self->priv->buf,
                                 (gsize) size,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 AFT_transfer_on_read,
                                 self);
    }
  else
    {
      evd_connection_lock_close (EVD_CONNECTION (self->priv->source_conn));
    }
}

static void
AFT_transfer_on_target_flushed (GObject      *obj,
                                    GAsyncResult *res,
                                    gpointer      user_data)
{
  AFTTransfer *self = user_data;
  GError *error = NULL;

  if (! g_output_stream_flush_finish (G_OUTPUT_STREAM (obj), res, &error))
    {
      /* if the stream was closed during flush it is not exactly an error,
         since it can be expected depending on how fast the target closed
         the connection after receiving all the body content */
      if (error->code != G_IO_ERROR_CLOSED)
        {
          g_printerr ("ERROR flushing target: %s\n", error->message);
          g_error_free (error);
        }
    }

  self->priv->status = AFT_TRANSFER_STATUS_COMPLETED;

  g_signal_handlers_disconnect_by_func (self->priv->target_conn,
                                        AFT_transfer_on_target_can_write,
                                        self);

  AFT_transfer_complete (self);

  g_object_unref (self);
}

static void
AFT_transfer_flush_target (AFTTransfer *self)
{
  GOutputStream *stream;

  stream = g_io_stream_get_output_stream (G_IO_STREAM (self->priv->target_conn));

  g_signal_handlers_disconnect_by_func (self->priv->target_conn,
                                        target_connection_on_close,
                                        self);

  g_object_ref (self);
  g_output_stream_flush_async (stream,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               AFT_transfer_on_target_flushed,
                               self);
}

static void
AFT_transfer_complete (AFTTransfer *self)
{
  g_signal_handlers_disconnect_by_func (self->priv->target_conn,
                                        target_connection_on_close,
                                        self);
  if (self->priv->source_conn != NULL)
    {
      g_signal_handlers_disconnect_by_func (self->priv->source_conn,
                                            source_connection_on_close,
                                            self);

      if (self->priv->status != AFT_TRANSFER_STATUS_COMPLETED &&
          ! g_io_stream_is_closed (G_IO_STREAM (self->priv->source_conn)))
        {
          g_io_stream_close (G_IO_STREAM (self->priv->source_conn), NULL, NULL);
        }
    }

  if (self->priv->status != AFT_TRANSFER_STATUS_COMPLETED &&
      ! g_io_stream_is_closed (G_IO_STREAM (self->priv->target_conn)))
    {
      g_io_stream_close (G_IO_STREAM (self->priv->target_conn), NULL, NULL);
    }

  if (self->priv->result != NULL)
    {
      g_simple_async_result_complete_in_idle (self->priv->result);
      g_object_unref (self->priv->result);
      self->priv->result = NULL;
    }
}

static gboolean
on_transfer_start_timeout (gpointer user_data)
{
  AFTTransfer *self = AFT_TRANSFER (user_data);

  self->priv->timeout_src_id = 0;

  if (self->priv->result != NULL)
    {
      g_simple_async_result_set_error (self->priv->result,
                                       G_IO_ERROR,
                                       G_IO_ERROR_TIMED_OUT,
                                       "Timeout starting transfer");

      evd_web_service_respond (self->priv->web_service,
                               self->priv->target_conn,
                               SOUP_STATUS_REQUEST_TIMEOUT,
                               NULL,
                               NULL,
                               0,
                               NULL);

      AFT_transfer_flush_target (self);
    }

  return FALSE;
}

/* public methods */

AFTTransfer *
AFT_transfer_new (AFTSource       *source,
                      EvdWebService       *web_service,
                      EvdHttpConnection   *target_conn,
                      const gchar         *action,
                      gboolean             is_chunked,
                      SoupRange           *range,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  AFTTransfer *self;

  g_return_val_if_fail (AFT_IS_SOURCE (source), NULL);
  g_return_val_if_fail (EVD_IS_WEB_SERVICE (web_service), NULL);
  g_return_val_if_fail (EVD_IS_HTTP_CONNECTION (target_conn), NULL);

  self = g_object_new (AFT_TYPE_TRANSFER, NULL);

  self->priv->result = g_simple_async_result_new (G_OBJECT (self),
                                                  callback,
                                                  user_data,
                                                  AFT_transfer_new);

  self->priv->source = g_object_ref (source);
  self->priv->web_service = g_object_ref (web_service);

  if (cancellable != NULL)
    self->priv->cancellable = g_object_ref (cancellable);

  self->priv->target_conn = g_object_ref (target_conn);
  g_signal_connect (target_conn,
                    "close",
                    G_CALLBACK (target_connection_on_close),
                    self);

  self->priv->id = evd_uuid_new ();
  self->priv->action = g_strdup (action);
  self->priv->is_chunked = is_chunked;
  if (is_chunked)
    {
      g_warning ("Chunked!\n");

      self->priv->byte_range.start = MAX (range->start, 0);
      self->priv->byte_range.end = range->end;
    }

  self->priv->status = AFT_TRANSFER_STATUS_NOT_STARTED;

  self->priv->timeout_src_id = evd_timeout_add (NULL,
                                                START_TIMEOUT,
                                                G_PRIORITY_LOW,
                                                on_transfer_start_timeout,
                                                self);

  return self;
}

const gchar *
AFT_transfer_get_id (AFTTransfer *self)
{
  g_return_val_if_fail (AFT_IS_TRANSFER (self), NULL);

  return self->priv->id;
}

void
AFT_transfer_set_source_conn (AFTTransfer   *self,
                                  EvdHttpConnection *conn)
{
  g_return_if_fail (AFT_IS_TRANSFER (self));
  g_return_if_fail (EVD_IS_HTTP_CONNECTION (conn));

  self->priv->source_conn = g_object_ref (conn);

  g_signal_connect (self->priv->source_conn,
                    "close",
                    G_CALLBACK (source_connection_on_close),
                    self);
}

void
AFT_transfer_start (AFTTransfer *self)
{
  SoupMessageHeaders *headers;
  GError *error = NULL;
  gint status = SOUP_STATUS_OK;

  g_return_if_fail (AFT_IS_TRANSFER (self));
  g_return_if_fail (self->priv->source_conn != NULL);

  if (self->priv->timeout_src_id != 0)
    {
      g_source_remove (self->priv->timeout_src_id);
      self->priv->timeout_src_id = 0;
    }

  headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_RESPONSE);

  if (g_strcmp0 (self->priv->action, "open") != 0)
    {
      gchar *st;
      gchar *decoded_file_name;

      decoded_file_name = soup_uri_decode (AFT_source_get_name (self->priv->source));
      st = g_strdup_printf ("attachment; filename=\"%s\"", decoded_file_name);
      g_free (decoded_file_name);
      soup_message_headers_replace (headers, "Content-disposition", st);
      g_free (st);
    }

  /* update transfer len */
  if (self->priv->is_chunked)
    {
      if (self->priv->byte_range.end == -1)
        self->priv->byte_range.end =
          AFT_source_get_size (self->priv->source) - 1;
      else
        self->priv->byte_range.end =
          MIN (self->priv->byte_range.end,
               AFT_source_get_size (self->priv->source) - 1);

      self->priv->transfer_len =
        self->priv->byte_range.end - self->priv->byte_range.start + 1;
    }
  else
    {
      self->priv->transfer_len = AFT_source_get_size (self->priv->source);
    }

  /* prepare target response headers */
  soup_message_headers_set_content_length (headers, self->priv->transfer_len);
  soup_message_headers_set_content_type (headers,
                           AFT_source_get_content_type (self->priv->source),
                           NULL);
  soup_message_headers_replace (headers, "Connection", "keep-alive");

  if (self->priv->is_chunked)
    {
      soup_message_headers_set_content_range (headers,
                                              self->priv->byte_range.start,
                                              self->priv->byte_range.end,
                                              AFT_source_get_size (self->priv->source));
      status = SOUP_STATUS_PARTIAL_CONTENT;
    }

  if (! evd_web_service_respond_headers (self->priv->web_service,
                                         self->priv->target_conn,
                                         status,
                                         headers,
                                         &error))
    {
      g_printerr ("Error sending transfer target headers: %s\n", error->message);
      g_error_free (error);

      /* @TODO: abort the transfer */
    }
  else
    {
      if (self->priv->buf == NULL)
        self->priv->buf = g_slice_alloc (BLOCK_SIZE);

      g_signal_connect (self->priv->target_conn,
                        "write",
                        G_CALLBACK (AFT_transfer_on_target_can_write),
                        self);

      self->priv->status = AFT_TRANSFER_STATUS_ACTIVE;

      AFT_transfer_read (self);
    }

  soup_message_headers_free (headers);
}

gboolean
AFT_transfer_finish (AFTTransfer  *self,
                         GAsyncResult     *result,
                         GError          **error)
{
  g_return_val_if_fail (AFT_IS_TRANSFER (self), FALSE);
  g_return_val_if_fail (g_simple_async_result_is_valid (result,
                                                        G_OBJECT (self),
                                                        AFT_transfer_new),
                        FALSE);

  return ! g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
                                                  error);
}

void
AFT_transfer_set_target_peer (AFTTransfer *self, EvdPeer *peer)
{
  g_return_if_fail (AFT_IS_TRANSFER (self));
  g_return_if_fail (EVD_IS_PEER (peer));

  if (self->priv->target_peer != NULL)
    g_object_unref (self->priv->target_peer);

  self->priv->target_peer = peer;

  if (peer != NULL)
    g_object_ref (peer);
}

void
AFT_transfer_get_status (AFTTransfer *self,
                             guint           *status,
                             gsize           *transferred,
                             gdouble         *bandwidth)
{
  g_return_if_fail (AFT_IS_TRANSFER (self));

  if (status != NULL)
    *status = self->priv->status;

  if (transferred != NULL)
    *transferred = self->priv->transferred;

  if (bandwidth != NULL)
    {
      if (self->priv->source_conn != NULL)
        {
          EvdStreamThrottle *throttle;

          throttle =
            evd_io_stream_get_input_throttle (EVD_IO_STREAM (self->priv->source_conn));

          *bandwidth = evd_stream_throttle_get_actual_bandwidth (throttle);
        }
      else
        {
          *bandwidth = 0;
        }
    }
}

void
AFT_transfer_cancel (AFTTransfer *self)
{
  g_return_if_fail (AFT_IS_TRANSFER (self));

  if (self->priv->result == NULL)
    return;

  g_simple_async_result_set_error (self->priv->result,
                                   G_IO_ERROR,
                                   G_IO_ERROR_CANCELLED,
                                   "Transfer cancelled");

  self->priv->status = AFT_TRANSFER_STATUS_SOURCE_ABORTED;

  AFT_transfer_complete (self);
}
