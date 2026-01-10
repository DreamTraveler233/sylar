/**
 * @file group_member.hpp
 * @brief 数据模型实体
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 数据模型实体。
 */

#pragma once
#include <string>

#include "core/base/macro.hpp"

namespace IM::model {

struct GroupMember {
    uint64_t id = 0;
    uint64_t group_id = 0;
    uint64_t user_id = 0;
    int role = 1;                // 角色 1:普通成员 2:管理员 3:群主
    std::string visit_card;      // 群名片
    std::string no_speak_until;  // 禁言截止时间
    std::string joined_at;       // 加入时间
    std::string created_at;      // 创建时间
    std::string updated_at;      // 更新时间
    std::string deleted_at;      // 删除时间
};

}  // namespace IM::model
