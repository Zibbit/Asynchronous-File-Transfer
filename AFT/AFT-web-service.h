


#ifndef __AFT_WEB_SERVICE_H__
#define __AFT_WEB_SERVICE_H__

#include <evd.h>

G_BEGIN_DECLS

typedef struct _AFTWebService AFTWebService;
typedef struct _AFTWebServiceClass AFTWebServiceClass;
typedef struct _AFTWebServicePrivate AFTWebServicePrivate;

struct _AFTWebService
{
  EvdWebService parent;

  AFTWebServicePrivate *priv;
};

struct _AFTWebServiceClass
{
  EvdWebServiceClass parent_class;
};

typedef void (* AFTWebServiceContentRequestCb) (AFTWebService *self,
                                                    const gchar       *content_id,
                                                    EvdHttpConnection *conn,
                                                    EvdHttpRequest    *request,
                                                    gpointer           user_data);

#define AFT_TYPE_WEB_SERVICE           (AFT_web_service_get_type ())
#define AFT_WEB_SERVICE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), AFT_TYPE_WEB_SERVICE, AFTWebService))
#define AFT_WEB_SERVICE_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), AFT_TYPE_WEB_SERVICE, AFTWebServiceClass))
#define AFT_IS_WEB_SERVICE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AFT_TYPE_WEB_SERVICE))
#define AFT_IS_WEB_SERVICE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), AFT_TYPE_WEB_SERVICE))
#define AFT_WEB_SERVICE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), AFT_TYPE_WEB_SERVICE, AFTWebServiceClass))


GType               AFT_web_service_get_type                (void) G_GNUC_CONST;

AFTWebService * AFT_web_service_new                     (GKeyFile                           *config,
                                                                 AFTWebServiceContentRequestCb   content_req_cb,
                                                                 gpointer                            user_data,
                                                                 GError                            **error);

EvdTransport *      AFT_web_service_get_transport           (AFTWebService *self);

#ifdef ENABLE_TESTS

#endif /* ENABLE_TESTS */

G_END_DECLS

#endif /* __AFT_WEB_SERVICE_H__ */
