#ifndef __IM_APP_RPC_TALK_REPOSITORY_RPC_CLIENT_HPP__
#define __IM_APP_RPC_TALK_REPOSITORY_RPC_CLIENT_HPP__

#include <atomic>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/config/config.hpp"
#include "core/io/lock.hpp"
#include "core/net/rock/rock_stream.hpp"
#include "domain/repository/talk_repository.hpp"

namespace IM::app::rpc {

// RPC-backed ITalkRepository implementation.
// Only the readonly query methods needed by WsGatewayModule are supported.
class TalkRepositoryRpcClient : public IM::domain::repository::ITalkRepository {
   public:
    TalkRepositoryRpcClient();
    ~TalkRepositoryRpcClient() override = default;

    bool findOrCreateSingleTalk(const std::shared_ptr<IM::MySQL>& /*db*/, uint64_t /*uid1*/,
                                uint64_t /*uid2*/, uint64_t& /*out_talk_id*/,
                                std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool findOrCreateGroupTalk(const std::shared_ptr<IM::MySQL>& /*db*/, uint64_t /*group_id*/,
                               uint64_t& /*out_talk_id*/, std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool getSingleTalkId(const uint64_t /*uid1*/, const uint64_t /*uid2*/, uint64_t& /*out_talk_id*/,
                         std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool getGroupTalkId(const uint64_t group_id, uint64_t& out_talk_id,
                        std::string* err = nullptr) override;

    bool nextSeq(const std::shared_ptr<IM::MySQL>& /*db*/, uint64_t /*talk_id*/, uint64_t& /*seq*/,
                 std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool getSessionListByUserId(const uint64_t /*user_id*/, std::vector<dto::TalkSessionItem>& /*out*/,
                                std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool setSessionTop(const uint64_t /*user_id*/, const uint64_t /*to_from_id*/,
                       const uint8_t /*talk_mode*/, const uint8_t /*action*/,
                       std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool setSessionDisturb(const uint64_t /*user_id*/, const uint64_t /*to_from_id*/,
                           const uint8_t /*talk_mode*/, const uint8_t /*action*/,
                           std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool createSession(const std::shared_ptr<IM::MySQL>& /*db*/, const model::TalkSession& /*session*/,
                       std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool getSessionByUserId(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*user_id*/,
                            dto::TalkSessionItem& /*out*/, const uint64_t /*to_from_id*/,
                            const uint8_t /*talk_mode*/, std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool deleteSession(const uint64_t /*user_id*/, const uint64_t /*to_from_id*/, const uint8_t /*talk_mode*/,
                       std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool deleteSession(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*user_id*/,
                       const uint64_t /*to_from_id*/, const uint8_t /*talk_mode*/,
                       std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool clearSessionUnreadNum(const uint64_t /*user_id*/, const uint64_t /*to_from_id*/,
                               const uint8_t /*talk_mode*/, std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool bumpOnNewMessage(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*talk_id*/,
                          const uint64_t /*sender_user_id*/, const std::string& /*last_msg_id*/,
                          const uint16_t /*last_msg_type*/, const std::string& /*last_msg_digest*/,
                          std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool updateLastMsgForUser(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*user_id*/,
                              const uint64_t /*talk_id*/, const std::optional<std::string>& /*last_msg_id*/,
                              const std::optional<uint16_t>& /*last_msg_type*/,
                              const std::optional<uint64_t>& /*last_sender_id*/,
                              const std::optional<std::string>& /*last_msg_digest*/,
                              std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool listUsersByLastMsg(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*talk_id*/,
                            const std::string& /*last_msg_id*/, std::vector<uint64_t>& /*out_user_ids*/,
                            std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool listUsersByTalkId(const uint64_t talk_id, std::vector<uint64_t>& out_user_ids,
                           std::string* err = nullptr) override;

    bool editRemarkWithConn(const std::shared_ptr<IM::MySQL>& /*db*/, const uint64_t /*user_id*/,
                            const uint64_t /*to_from_id*/, const std::string& /*remark*/,
                            std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool updateSessionAvatarByTargetUserWithConn(const std::shared_ptr<IM::MySQL>& /*db*/,
                                                 const uint64_t /*target_user_id*/,
                                                 const std::string& /*avatar*/,
                                                 std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool listUsersByTargetUserWithConn(const std::shared_ptr<IM::MySQL>& /*db*/,
                                       const uint64_t /*target_user_id*/,
                                       std::vector<uint64_t>& /*out_user_ids*/,
                                       std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

    bool updateSessionAvatarByTargetUser(const uint64_t /*target_user_id*/, const std::string& /*avatar*/,
                                         std::string* err = nullptr) override {
        if (err) *err = "not supported";
        return false;
    }

   private:
    IM::RockResult::ptr rockJsonRequest(const std::string& ip_port, uint32_t cmd,
                                       const Json::Value& body, uint32_t timeout_ms);
    std::string resolveSvcTalkAddr();

   private:
    IM::ConfigVar<std::string>::ptr m_rpc_addr;

    std::atomic<uint32_t> m_sn{1};
    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
};

}  // namespace IM::app::rpc

#endif  // __IM_APP_RPC_TALK_REPOSITORY_RPC_CLIENT_HPP__
