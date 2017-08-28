

#include "AFT-source.h"

G_DEFINE_TYPE (AFTSource, AFT_source, G_TYPE_OBJECT)

#define AFT_SOURCE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                         AFT_SOURCE_TYPE, \
                                         AFTSourcePrivate))

/* private data */
struct _AFTSourcePrivate
{
  gchar *id;
  gchar *signature;
  EvdPeer *peer;
  guint flags;
  gchar *name;
  gchar *type;
  gsize size;
  gchar **tags;

  GCancellable *cancellable;
  GError *error;
};

static void     AFT_source_class_init         (AFTSourceClass *class);
static void     AFT_source_init               (AFTSource *self);

static void     AFT_source_finalize           (GObject *obj);
static void     AFT_source_dispose            (GObject *obj);

static void
AFT_source_class_init (AFTSourceClass *class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (class);

  obj_class->dispose = AFT_source_dispose;
  obj_class->finalize = AFT_source_finalize;

  g_type_class_add_private (obj_class, sizeof (AFTSourcePrivate));
}

static void
AFT_source_init (AFTSource *self)
{
  AFTSourcePrivate *priv;

  priv = AFT_SOURCE_GET_PRIVATE (self);
  self->priv = priv;

  priv->id = NULL;
  priv->signature = NULL;
  priv->tags = NULL;

  priv->cancellable = g_cancellable_new ();
  priv->error = NULL;
}

static void
AFT_source_dispose (GObject *obj)
{
  AFTSource *self = AFT_SOURCE (obj);

  if (self->priv->peer != NULL)
    {
      g_object_unref (self->priv->peer);
      self->priv->peer = NULL;
    }

  G_OBJECT_CLASS (AFT_source_parent_class)->dispose (obj);
}

static void
AFT_source_finalize (GObject *obj)
{
  AFTSource *self = AFT_SOURCE (obj);

  g_free (self->priv->id);
  g_free (self->priv->signature);
  g_free (self->priv->name);
  g_free (self->priv->type);
  g_strfreev (self->priv->tags);

  g_object_unref (self->priv->cancellable);

  if (self->priv->error != NULL)
    g_error_free (self->priv->error);

  G_OBJECT_CLASS (AFT_source_parent_class)->finalize (obj);
}

/* public methods */

AFTSource *
AFT_source_new (EvdPeer      *peer,
                    const gchar  *name,
                    const gchar  *type,
                    gsize         size,
                    guint         flags,
                    const gchar **tags)
{
  AFTSource *self;

  g_return_val_if_fail (EVD_IS_PEER (peer) || peer == NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (type != NULL, NULL);

  self = g_object_new (AFT_SOURCE_TYPE, NULL);

  if (peer != NULL)
    self->priv->peer = g_object_ref (peer);

  self->priv->name = g_strdup (name);
  self->priv->type = g_strdup (type);
  self->priv->size = size;
  self->priv->flags = flags;
  if (tags != NULL)
    self->priv->tags = g_strdupv ((gchar **) tags);

  return self;
}

void
AFT_source_set_peer (AFTSource *self, EvdPeer *peer)
{
  g_return_if_fail (AFT_IS_SOURCE (self));
  g_return_if_fail (EVD_IS_PEER (peer));

  if (self->priv->peer != NULL)
    g_object_unref (self->priv->peer);
  self->priv->peer = g_object_ref (peer);
}

EvdPeer *
AFT_source_get_peer (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->peer;
}

const gchar *
AFT_source_get_name (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->name;
}

const gchar *
AFT_source_get_content_type (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->type;
}

void
AFT_source_set_size (AFTSource *self, gsize size)
{
  g_return_if_fail (AFT_IS_SOURCE (self));

  self->priv->size = size;
}

gsize
AFT_source_get_size (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), 0);

  return self->priv->size;
}

guint
AFT_source_get_flags (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), 0);

  return self->priv->flags;
}

const gchar **
AFT_source_get_tags (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), 0);

  return (const gchar **) self->priv->tags;
}

void
AFT_source_set_id (AFTSource *self, const gchar *id)
{
  g_return_if_fail (AFT_IS_SOURCE (self));
  g_return_if_fail (id != NULL && strlen (id) > 6);

  g_free (self->priv->id);
  self->priv->id = g_strdup (id);
}

const gchar *
AFT_source_get_id (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->id;
}

void
AFT_source_set_signature (AFTSource *self, const gchar *signature)
{
  g_return_if_fail (AFT_IS_SOURCE (self));
  g_return_if_fail (signature != NULL);

  g_free (self->priv->signature);
  self->priv->signature = g_strdup (signature);
}

const gchar *
AFT_source_get_signature (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->signature;
}

gboolean
AFT_source_is_chunkable (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), FALSE);

  return (self->priv->flags & AFT_SOURCE_FLAGS_CHUNKABLE);
}

GCancellable *
AFT_source_get_cancellable (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->cancellable;
}

void
AFT_source_take_error (AFTSource *self, GError *error)
{
  g_return_if_fail (AFT_IS_SOURCE (self));

  self->priv->error = error;
}

GError *
AFT_source_get_error (AFTSource *self)
{
  g_return_val_if_fail (AFT_IS_SOURCE (self), NULL);

  return self->priv->error;
}
