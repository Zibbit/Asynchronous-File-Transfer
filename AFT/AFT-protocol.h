

#ifndef __AFT_PROTOCOL_H__
#define __AFT_PROTOCOL_H__

#include <evd.h>

#include "AFT-source.h"
#include "AFT-transfer.h"

G_BEGIN_DECLS

typedef struct _AFTProtocol AFTProtocol;
typedef struct _AFTProtocolClass AFTProtocolClass;
typedef struct _AFTProtocolPrivate AFTProtocolPrivate;

typedef struct
{
  gboolean (* register_source)   (AFTProtocol  *self,
                                  EvdPeer          *peer,
                                  AFTSource    *source,
                                  GError          **error,
                                  gpointer          user_data);
  gboolean (* unregister_source) (AFTProtocol  *self,
                                  EvdPeer          *peer,
                                  const gchar      *id,
                                  gboolean          gracefully,
                                  gpointer          user_data);

  void     (* content_request)   (AFTProtocol    *self,
                                  AFTSource      *source,
                                  EvdHttpConnection  *conn,
                                  const gchar        *action,
                                  const gchar        *peer_id,
                                  gboolean            is_chunked,
                                  SoupRange          *byte_range,
                                  gpointer            user_data);
  void     (* content_push)      (AFTProtocol    *self,
                                  AFTTransfer    *transfer,
                                  EvdHttpConnection  *conn,
                                  gpointer            user_data);

  void     (* seeder_push_request) (AFTProtocol *self,
                                    GAsyncResult    *result,
                                    const gchar     *source_id,
                                    const gchar     *transfer_id,
                                    gboolean         is_chunked,
                                    SoupRange       *byte_range,
                                    gpointer         user_data);

} AFTProtocolVTable;

struct _AFTProtocol
{
  GObject parent;

  AFTProtocolPrivate *priv;
};

struct _AFTProtocolClass
{
  GObjectClass parent_class;
};

#define AFT_TYPE_PROTOCOL           (AFT_protocol_get_type ())
#define AFT_PROTOCOL(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), AFT_TYPE_PROTOCOL, AFTProtocol))
#define AFT_PROTOCOL_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), AFT_TYPE_PROTOCOL, AFTProtocolClass))
#define AFT_IS_PROTOCOL(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AFT_TYPE_PROTOCOL))
#define AFT_IS_PROTOCOL_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), AFT_TYPE_PROTOCOL))
#define AFT_PROTOCOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), AFT_TYPE_PROTOCOL, AFTProtocolClass))


GType             AFT_protocol_get_type                (void) G_GNUC_CONST;

AFTProtocol * AFT_protocol_new                     (AFTProtocolVTable *vtable,
                                                            gpointer               user_data,
                                                            GDestroyNotify         user_data_free_func);

EvdJsonrpc *      AFT_protocol_get_rpc                 (AFTProtocol *self);

gboolean          AFT_protocol_handle_content_request  (AFTProtocol    *self,
                                                            AFTSource      *source,
                                                            EvdWebService      *web_service,
                                                            EvdHttpConnection  *conn,
                                                            EvdHttpRequest     *request,
                                                            GError            **error);
gboolean          AFT_protocol_handle_content_push     (AFTProtocol    *self,
                                                            AFTTransfer    *transfer,
                                                            EvdWebService      *web_service,
                                                            EvdHttpConnection  *conn,
                                                            EvdHttpRequest     *request,
                                                            GError            **error);

gboolean          AFT_protocol_request_content         (AFTProtocol  *self,
                                                            EvdPeer          *peer,
                                                            const gchar      *source_id,
                                                            const gchar      *transfer_id,
                                                            gboolean          is_chunked,
                                                            SoupRange        *byte_range,
                                                            GError          **error);

void              AFT_protocol_register_sources        (AFTProtocol     *self,
                                                            EvdPeer             *peer,
                                                            GList               *sources,
                                                            GCancellable        *cancellable,
                                                            GAsyncReadyCallback  callback,
                                                            gpointer             user_data);
gboolean          AFT_protocol_register_sources_finish (AFTProtocol  *self,
                                                            GAsyncResult     *result,
                                                            GList           **sources,
                                                            GError          **error);

G_END_DECLS

#endif /* __AFT_PROTOCOL_H__ */
