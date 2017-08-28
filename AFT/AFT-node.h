

#ifndef __AFT_NODE_H__
#define __AFT_NODE_H__

#include <evd.h>

#include "AFT-protocol.h"
#include "AFT-web-service.h"

G_BEGIN_DECLS

typedef struct _AFTNode AFTNode;
typedef struct _AFTNodeClass AFTNodeClass;
typedef struct _AFTNodePrivate AFTNodePrivate;

struct _AFTNode
{
  GObject parent;

  AFTNodePrivate *priv;
};

struct _AFTNodeClass
{
  GObjectClass parent_class;
};

#define AFT_TYPE_NODE           (AFT_node_get_type ())
#define AFT_NODE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), AFT_TYPE_NODE, AFTNode))
#define AFT_NODE_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), AFT_TYPE_NODE, AFTNodeClass))
#define AFT_IS_NODE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AFT_TYPE_NODE))
#define AFT_IS_NODE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), AFT_TYPE_NODE))
#define AFT_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), AFT_TYPE_NODE, AFTNodeClass))


GType               AFT_node_get_type              (void) G_GNUC_CONST;

AFTNode *       AFT_node_new                   (GKeyFile  *config,
                                                        GError   **error);

const gchar *       AFT_node_get_id                (AFTNode  *self);

AFTWebService * AFT_node_get_web_service       (AFTNode *self);

#ifdef ENABLE_TESTS

AFTProtocol *   AFT_node_get_protocol          (AFTNode *self);

GList *             AFT_node_get_all_sources       (AFTNode *self);

#endif /* ENABLE_TESTS */

G_END_DECLS

#endif /* __AFT_NODE_H__ */
