/**
 * @file http_session.hpp
 * @brief 网络通信相关
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 网络通信相关。
 */

#ifndef __IM_NET_HTTP_HTTP_SESSION_HPP__
#define __IM_NET_HTTP_HTTP_SESSION_HPP__

#include "core/base/memory_pool.hpp"
#include "core/net/streams/socket_stream.hpp"

#include "http.hpp"

namespace IM::http {
/**
 * @brief HTTPSession封装
 */
class HttpSession : public SocketStream {
   public:
    /// 智能指针类型定义
    typedef std::shared_ptr<HttpSession> ptr;

    /**
     * @brief 构造函数
     * @param[in] sock Socket类型
     * @param[in] owner 是否托管
     */
    HttpSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 接收HTTP请求
     * @return 返回HttpRequest对象的智能指针
     * @details 该函数负责从Socket中接收HTTP请求数据，并将其解析为HttpRequest对象
     *          包括解析HTTP请求行、请求头和请求体
     */
    HttpRequest::ptr recvRequest();

    /**
     * @brief 发送HTTP响应
     * @param[in] rsp HTTP响应
     * @return >0 发送成功
     *         =0 对方关闭
     *         <0 Socket异常
     */
    int sendResponse(HttpResponse::ptr rsp);

    int read(void *buffer, size_t length) override;
    int read(ByteArray::ptr ba, size_t length) override;

   protected:
    std::string m_leftoverBuf;
    // Per-session reusable pool for short-lived buffers (per request/message).
    // Only use it for trivially destructible / raw memory.
    IM::NgxMemPool m_reqPool;
};
}  // namespace IM::http

#endif  // __IM_NET_HTTP_HTTP_SESSION_HPP__