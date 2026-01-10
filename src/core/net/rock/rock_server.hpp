/**
 * @file rock_server.hpp
 * @brief 网络通信相关
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 网络通信相关。
 */

#ifndef __IM_NET_ROCK_ROCK_SERVER_HPP__
#define __IM_NET_ROCK_ROCK_SERVER_HPP__

#include "core/net/core/tcp_server.hpp"

#include "rock_stream.hpp"

namespace IM {
class RockServer : public TcpServer {
   public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string &type = "rock", IOManager *worker = IOManager::GetThis(),
               IOManager *io_worker = IOManager::GetThis(), IOManager *accept_worker = IOManager::GetThis());

   protected:
    virtual void handleClient(Socket::ptr client) override;
};

}  // namespace IM

#endif  // __IM_NET_ROCK_ROCK_SERVER_HPP__
