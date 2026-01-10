/**
 * @file config_servlet.hpp
 * @brief 网络通信相关
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 网络通信相关。
 */

#ifndef __IM_NET_HTTP_SERVLETS_CONFIG_SERVLET_HPP__
#define __IM_NET_HTTP_SERVLETS_CONFIG_SERVLET_HPP__

#include "core/net/http/http_servlet.hpp"

namespace IM::http {

class ConfigServlet : public Servlet {
   public:
    ConfigServlet();
    virtual int32_t handle(HttpRequest::ptr request, HttpResponse::ptr response, HttpSession::ptr session) override;
};

}  // namespace IM::http

#endif  // __IM_NET_HTTP_SERVLETS_CONFIG_SERVLET_HPP__
