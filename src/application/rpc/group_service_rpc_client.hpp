/**
 * @file group_service_rpc_client.hpp
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

#include "domain/service/group_service.hpp"

namespace IM::app::rpc {

class GroupServiceRpcClient final : public IM::domain::service::IGroupService {
   public:
    GroupServiceRpcClient();

    Result<uint64_t> CreateGroup(uint64_t user_id, const std::string &name,
                                 const std::vector<uint64_t> &member_ids) override;
    Result<void> DismissGroup(uint64_t user_id, uint64_t group_id) override;
    Result<IM::dto::GroupDetail> GetGroupDetail(uint64_t user_id, uint64_t group_id) override;
    Result<std::vector<IM::dto::GroupItem>> GetGroupList(uint64_t user_id) override;
    Result<void> UpdateGroupSetting(uint64_t user_id, uint64_t group_id, const std::string &name,
                                    const std::string &avatar, const std::string &profile) override;
    Result<void> HandoverGroup(uint64_t user_id, uint64_t group_id, uint64_t new_owner_id) override;
    Result<void> AssignAdmin(uint64_t user_id, uint64_t group_id, uint64_t target_id, int action) override;
    Result<void> MuteGroup(uint64_t user_id, uint64_t group_id, int action) override;
    Result<void> OvertGroup(uint64_t user_id, uint64_t group_id, int action) override;
    Result<std::pair<std::vector<IM::dto::GroupOvertItem>, bool>> GetOvertGroupList(int page,
                                                                                    const std::string &name) override;

    Result<std::vector<IM::dto::GroupMemberItem>> GetGroupMemberList(uint64_t user_id, uint64_t group_id) override;
    Result<void> InviteGroup(uint64_t user_id, uint64_t group_id, const std::vector<uint64_t> &member_ids) override;
    Result<void> RemoveMember(uint64_t user_id, uint64_t group_id, const std::vector<uint64_t> &member_ids) override;
    Result<void> SecedeGroup(uint64_t user_id, uint64_t group_id) override;
    Result<void> UpdateMemberRemark(uint64_t user_id, uint64_t group_id, const std::string &remark) override;
    Result<void> MuteMember(uint64_t user_id, uint64_t group_id, uint64_t target_id, int action) override;

    Result<void> CreateApply(uint64_t user_id, uint64_t group_id, const std::string &remark) override;
    Result<void> AgreeApply(uint64_t user_id, uint64_t apply_id) override;
    Result<void> DeclineApply(uint64_t user_id, uint64_t apply_id, const std::string &remark) override;
    Result<std::vector<IM::dto::GroupApplyItem>> GetApplyList(uint64_t user_id, uint64_t group_id) override;
    Result<std::vector<IM::dto::GroupApplyItem>> GetUserApplyList(uint64_t user_id) override;
    Result<int> GetUnreadApplyCount(uint64_t user_id) override;

    Result<uint64_t> CreateVote(uint64_t user_id, uint64_t group_id, const std::string &title, int answer_mode,
                                int is_anonymous, const std::vector<std::string> &options) override;
    Result<std::vector<IM::dto::GroupVoteItem>> GetVoteList(uint64_t user_id, uint64_t group_id) override;
    Result<IM::dto::GroupVoteDetail> GetVoteDetail(uint64_t user_id, uint64_t vote_id) override;
    Result<void> CastVote(uint64_t user_id, uint64_t vote_id, const std::vector<std::string> &options) override;
    Result<void> FinishVote(uint64_t user_id, uint64_t vote_id) override;

    Result<void> EditNotice(uint64_t user_id, uint64_t group_id, const std::string &content) override;

   private:
    IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd, const Json::Value &body,
                                        uint32_t timeout_ms);
    std::string resolveSvcGroupAddr();

    bool parseGroupItem(const Json::Value &j, IM::dto::GroupItem &out);
    bool parseGroupDetail(const Json::Value &j, IM::dto::GroupDetail &out);
    bool parseGroupMemberItem(const Json::Value &j, IM::dto::GroupMemberItem &out);
    bool parseGroupApplyItem(const Json::Value &j, IM::dto::GroupApplyItem &out);
    bool parseGroupOvertItem(const Json::Value &j, IM::dto::GroupOvertItem &out);
    bool parseGroupVoteItem(const Json::Value &j, IM::dto::GroupVoteItem &out);
    bool parseGroupVoteDetail(const Json::Value &j, IM::dto::GroupVoteDetail &out);

   private:
    IM::ConfigVar<std::string>::ptr m_rpc_addr;

    IM::RWMutex m_mutex;
    std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
    std::atomic<uint32_t> m_sn{1};
};

}  // namespace IM::app::rpc
