/**
 * @file message_service_rpc_client.hpp
 * @brief RPC客户端实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 RPC客户端实现。
 */

#ifndef __IM_APP_RPC_MESSAGE_SERVICE_RPC_CLIENT_HPP__
#define __IM_APP_RPC_MESSAGE_SERVICE_RPC_CLIENT_HPP__

#include <atomic>
#include <functional>
#include <unordered_map>

#include "core/config/config.hpp"
#include "core/io/lock.hpp"
#include "core/net/core/address.hpp"
#include "core/net/rock/rock_stream.hpp"
#include "core/util/json_util.hpp"

#include "infra/module/module.hpp"

#include "domain/service/message_service.hpp"

namespace IM::app::rpc {

class MessageServiceRpcClient : public IM::domain::service::IMessageService {
   public:
    MessageServiceRpcClient();

    Result<IM::dto::MessagePage> LoadRecords(const uint64_t current_user_id, const uint8_t talk_mode,
                                             const uint64_t to_from_id, uint64_t cursor, uint32_t limit) override;
    Result<IM::dto::MessagePage> LoadHistoryRecords(const uint64_t current_user_id, const uint8_t talk_mode,
                                                    const uint64_t to_from_id, const uint16_t msg_type, uint64_t cursor,
                                                    uint32_t limit) override;
    Result<std::vector<IM::dto::MessageRecord>> LoadForwardRecords(const uint64_t current_user_id,
                                                                   const uint8_t talk_mode,
                                                                   const std::vector<std::string> &msg_ids) override;
    Result<void> DeleteMessages(const uint64_t current_user_id, const uint8_t talk_mode, const uint64_t to_from_id,
                                const std::vector<std::string> &msg_ids) override;
    Result<void> DeleteAllMessagesInTalkForUser(const uint64_t current_user_id, const uint8_t talk_mode,
                                                const uint64_t to_from_id) override;
    Result<void> ClearTalkRecords(const uint64_t current_user_id, const uint8_t talk_mode,
                                  const uint64_t to_from_id) override;
    Result<void> RevokeMessage(const uint64_t current_user_id, const uint8_t talk_mode, const uint64_t to_from_id,
                               const std::string &msg_id) override;
    Result<IM::dto::MessageRecord> SendMessage(const uint64_t current_user_id, const uint8_t talk_mode,
                                               const uint64_t to_from_id, const uint16_t msg_type,
                                               const std::string &content_text, const std::string &extra,
                                               const std::string &quote_msg_id, const std::string &msg_id,
                                               const std::vector<uint64_t> &mentioned_user_ids) override;
    Result<void> UpdateMessageStatus(const uint64_t current_user_id, const uint8_t talk_mode, const uint64_t to_from_id,
                                     const std::string &msg_id, uint8_t status) override;

   private:
    uint64_t resolveTalkId(const uint8_t talk_mode, const uint64_t to_from_id) override;
    bool buildRecord(const IM::model::Message &msg, IM::dto::MessageRecord &out, std::string *err) override;
    bool GetTalkId(const uint64_t current_user_id, const uint8_t talk_mode, const uint64_t to_from_id,
                   uint64_t &talk_id, std::string &err) override;

    IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd, const Json::Value &body,
                                        uint32_t timeout_ms);
    std::string resolveSvcMessageAddr();

    Result<void> callVoid(uint32_t cmd, const std::function<void(Json::Value &)> &fill);

    static bool parseMessageRecord(const Json::Value &j, IM::dto::MessageRecord &out);
    static bool parseMessagePage(const Json::Value &j, IM::dto::MessagePage &out);

   private:
    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
    std::atomic<uint32_t> m_sn{1};

    IM::ConfigVar<std::string>::ptr m_rpc_addr;
};

}  // namespace IM::app::rpc

#endif  // __IM_APP_RPC_MESSAGE_SERVICE_RPC_CLIENT_HPP__
