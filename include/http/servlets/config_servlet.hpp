#ifndef __IM_HTTP_SERVLETS_CONFIG_SERVLET_HPP__
#define __IM_HTTP_SERVLETS_CONFIG_SERVLET_HPP__

#include "http/http_servlet.hpp"

namespace IM::http {

class ConfigServlet : public Servlet {
   public:
    ConfigServlet();
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response,
                           HttpSession::ptr session) override;
};

}  // namespace IM::http

#endif // __IM_HTTP_SERVLETS_CONFIG_SERVLET_HPP__
