

#ifndef _AFT_TRANSFER_H_
#define _AFT_TRANSFER_H_

#include <gio/gio.h>
#include <evd.h>

#include "AFT-source.h"

G_BEGIN_DECLS

typedef struct _AFTTransfer AFTTransfer;
typedef struct _AFTTransferClass AFTTransferClass;
typedef struct _AFTTransferPrivate AFTTransferPrivate;

struct _AFTTransfer
{
  GObject parent;

  AFTTransferPrivate *priv;
};

struct _AFTTransferClass
{
  GObjectClass parent_class;
};

typedef struct _AFTTransfer AFTTransfer;

typedef enum
{
  AFT_TRANSFER_STATUS_NOT_STARTED,
  AFT_TRANSFER_STATUS_ACTIVE,
  AFT_TRANSFER_STATUS_PAUSED,
  AFT_TRANSFER_STATUS_COMPLETED,
  AFT_TRANSFER_STATUS_SOURCE_ABORTED,
  AFT_TRANSFER_STATUS_TARGET_ABORTED,
  AFT_TRANSFER_STATUS_ERROR,
} AFTTransferStatus;

#define AFT_TYPE_TRANSFER           (AFT_transfer_get_type ())
#define AFT_TRANSFER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), AFT_TYPE_TRANSFER, AFTTransfer))
#define AFT_TRANSFER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), AFT_TYPE_TRANSFER, AFTTransferClass))
#define AFT_IS_TRANSFER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AFT_TYPE_TRANSFER))
#define AFT_IS_TRANSFER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), AFT_TYPE_TRANSFER))
#define AFT_TRANSFER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), AFT_TYPE_TRANSFER, AFTTransferClass))

GType             AFT_transfer_get_type              (void) G_GNUC_CONST;

AFTTransfer * AFT_transfer_new                   (AFTSource       *source,
                                                          EvdWebService       *web_service,
                                                          EvdHttpConnection   *target_conn,
                                                          const gchar         *action,
                                                          gboolean             is_chunked,
                                                          SoupRange           *range,
                                                          GCancellable        *cancellable,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);

const gchar *     AFT_transfer_get_id                (AFTTransfer *self);


void              AFT_transfer_start                 (AFTTransfer *self);
gboolean          AFT_transfer_finish                (AFTTransfer  *self,
                                                          GAsyncResult     *result,
                                                          GError          **error);

void              AFT_transfer_set_source_conn       (AFTTransfer   *self,
                                                          EvdHttpConnection *conn);

void              AFT_transfer_set_target_peer       (AFTTransfer *self,
                                                          EvdPeer         *peer);

void              AFT_transfer_get_status            (AFTTransfer *self,
                                                          guint           *status,
                                                          gsize           *transferred,
                                                          gdouble         *bandwidth);

void              AFT_transfer_cancel                (AFTTransfer *self);

#endif /* _AFT_TRANSFER_H_ */
