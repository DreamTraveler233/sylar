/**
 * @file talk_service_rpc_client.hpp
 * @brief RPC客户端实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 RPC客户端实现。
 */

#ifndef __IM_APP_RPC_TALK_SERVICE_RPC_CLIENT_HPP__
#define __IM_APP_RPC_TALK_SERVICE_RPC_CLIENT_HPP__

#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/config/config.hpp"
#include "core/io/lock.hpp"
#include "core/net/rock/rock_stream.hpp"

#include "domain/service/talk_service.hpp"

namespace IM::app::rpc {

class TalkServiceRpcClient : public IM::domain::service::ITalkService {
   public:
    TalkServiceRpcClient();
    ~TalkServiceRpcClient() override = default;

    Result<std::vector<IM::dto::TalkSessionItem>> getSessionListByUserId(const uint64_t user_id) override;

    Result<void> setSessionTop(const uint64_t user_id, const uint64_t to_from_id, const uint8_t talk_mode,
                               const uint8_t action) override;

    Result<void> setSessionDisturb(const uint64_t user_id, const uint64_t to_from_id, const uint8_t talk_mode,
                                   const uint8_t action) override;

    Result<IM::dto::TalkSessionItem> createSession(const uint64_t user_id, const uint64_t to_from_id,
                                                   const uint8_t talk_mode) override;

    Result<void> deleteSession(const uint64_t user_id, const uint64_t to_from_id, const uint8_t talk_mode) override;

    Result<void> clearSessionUnreadNum(const uint64_t user_id, const uint64_t to_from_id,
                                       const uint8_t talk_mode) override;

   private:
    IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd, const Json::Value &body,
                                        uint32_t timeout_ms);
    std::string resolveSvcTalkAddr();

    static bool parseTalkSessionItem(const Json::Value &j, IM::dto::TalkSessionItem &out);

   private:
    IM::ConfigVar<std::string>::ptr m_rpc_addr;

    std::atomic<uint32_t> m_sn{1};
    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
};

}  // namespace IM::app::rpc

#endif  // __IM_APP_RPC_TALK_SERVICE_RPC_CLIENT_HPP__
