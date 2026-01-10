/**
 * @file contact_service_rpc_client.hpp
 * @brief RPC客户端实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 RPC客户端实现。
 */

#pragma once

#include <atomic>
#include <jsoncpp/json/json.h>
#include <unordered_map>

#include "core/config/config.hpp"
#include "core/io/lock.hpp"
#include "core/net/rock/rock_stream.hpp"

#include "domain/service/contact_service.hpp"

namespace IM::app::rpc {

class ContactServiceRpcClient final : public IM::domain::service::IContactService {
   public:
    ContactServiceRpcClient();

    Result<IM::dto::TalkSessionItem> AgreeApply(const uint64_t user_id, const uint64_t apply_id,
                                                const std::string &remark) override;

    Result<IM::model::User> SearchByMobile(const std::string &mobile) override;

    Result<IM::dto::ContactDetails> GetContactDetail(const uint64_t user_id, const uint64_t target_id) override;

    Result<std::vector<IM::dto::ContactItem>> ListFriends(const uint64_t user_id) override;

    Result<void> CreateContactApply(const uint64_t apply_user_id, const uint64_t target_user_id,
                                    const std::string &remark) override;

    Result<uint64_t> GetPendingContactApplyCount(const uint64_t user_id) override;

    Result<std::vector<IM::dto::ContactApplyItem>> ListContactApplies(const uint64_t user_id) override;

    Result<void> RejectApply(const uint64_t handler_user_id, const uint64_t apply_user_id,
                             const std::string &remark) override;

    Result<void> EditContactRemark(const uint64_t user_id, const uint64_t contact_id,
                                   const std::string &remark) override;

    Result<void> DeleteContact(const uint64_t user_id, const uint64_t contact_id) override;

    Result<void> SaveContactGroup(const uint64_t user_id,
                                  const std::vector<std::tuple<uint64_t, uint64_t, std::string>> &groupItems) override;

    Result<std::vector<IM::dto::ContactGroupItem>> GetContactGroupLists(const uint64_t user_id) override;

    Result<void> ChangeContactGroup(const uint64_t user_id, const uint64_t contact_id,
                                    const uint64_t group_id) override;

   private:
    IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd, const Json::Value &body,
                                        uint32_t timeout_ms);
    std::string resolveSvcContactAddr();

    bool parseTalkSession(const Json::Value &j, IM::dto::TalkSessionItem &out);
    bool parseUser(const Json::Value &j, IM::model::User &out);
    bool parseContactItem(const Json::Value &j, IM::dto::ContactItem &out);
    bool parseContactApplyItem(const Json::Value &j, IM::dto::ContactApplyItem &out);
    bool parseContactGroupItem(const Json::Value &j, IM::dto::ContactGroupItem &out);
    bool parseContactDetails(const Json::Value &j, IM::dto::ContactDetails &out);

   private:
    IM::ConfigVar<std::string>::ptr m_rpc_addr;

    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
    std::atomic<uint32_t> m_sn{1};
};

}  // namespace IM::app::rpc
