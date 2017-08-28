

#ifndef __AFT_SOURCE_H__
#define __AFT_SOURCE_H__

#include <evd.h>

G_BEGIN_DECLS

typedef struct _AFTSource AFTSource;
typedef struct _AFTSourceClass AFTSourceClass;
typedef struct _AFTSourcePrivate AFTSourcePrivate;

typedef enum {
  AFT_SOURCE_FLAGS_NONE          = 0,
  AFT_SOURCE_FLAGS_PUBLIC        = 1 << 0,
  AFT_SOURCE_FLAGS_LIVE          = 1 << 1,
  AFT_SOURCE_FLAGS_REAL_TIME     = 1 << 2,
  AFT_SOURCE_FLAGS_CHUNKABLE     = 1 << 3,
  AFT_SOURCE_FLAGS_BIDIRECTIONAL = 1 << 4
} AFTSourceFlags;

struct _AFTSource
{
  EvdWebService parent;

  AFTSourcePrivate *priv;
};

struct _AFTSourceClass
{
  EvdWebServiceClass parent_class;
};

#define AFT_SOURCE_TYPE           (AFT_source_get_type ())
#define AFT_SOURCE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), AFT_SOURCE_TYPE, AFTSource))
#define AFT_SOURCE_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), AFT_SOURCE_TYPE, AFTSourceClass))
#define AFT_IS_SOURCE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AFT_SOURCE_TYPE))
#define AFT_IS_SOURCE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), AFT_SOURCE_TYPE))
#define AFT_SOURCE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), AFT_SOURCE_TYPE, AFTSourceClass))


GType             AFT_source_get_type                (void) G_GNUC_CONST;

AFTSource *   AFT_source_new                     (EvdPeer      *peer,
                                                          const gchar  *name,
                                                          const gchar  *type,
                                                          gsize         size,
                                                          guint         flags,
                                                          const gchar **tags);

void              AFT_source_set_peer                (AFTSource *self,
                                                          EvdPeer       *peer);
EvdPeer *         AFT_source_get_peer                (AFTSource *self);

const gchar *     AFT_source_get_name                (AFTSource  *self);
const gchar *     AFT_source_get_content_type        (AFTSource  *self);
void              AFT_source_set_size                (AFTSource *self,
                                                          gsize          size);
gsize             AFT_source_get_size                (AFTSource  *self);
guint             AFT_source_get_flags               (AFTSource  *self);
const gchar **    AFT_source_get_tags                (AFTSource  *self);

void              AFT_source_set_id                  (AFTSource *self,
                                                          const gchar   *id);
const gchar *     AFT_source_get_id                  (AFTSource *self);

void              AFT_source_set_signature           (AFTSource *self,
                                                          const gchar   *signature);
const gchar *     AFT_source_get_signature           (AFTSource *self);

gboolean          AFT_source_is_chunkable            (AFTSource *self);

GCancellable *    AFT_source_get_cancellable         (AFTSource *self);

void              AFT_source_take_error              (AFTSource *self,
                                                          GError        *error);
GError *          AFT_source_get_error               (AFTSource *self);

G_END_DECLS

#endif /* __AFT_SOURCE_H__ */
