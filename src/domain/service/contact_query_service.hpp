/**
 * @file contact_query_service.hpp
 * @brief 领域服务接口
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 领域服务接口。
 */

#ifndef __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__
#define __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__

#include <memory>

#include "dto/contact_dto.hpp"

#include "common/result.hpp"

namespace IM::domain::service {

// 仅用于跨域读取联系人的“查询”能力（避免把完整 IContactService 引入到其它服务）。
class IContactQueryService {
   public:
    using Ptr = std::shared_ptr<IContactQueryService>;
    virtual ~IContactQueryService() = default;

    virtual Result<IM::dto::ContactDetails> GetContactDetail(const uint64_t owner_id, const uint64_t target_id) = 0;
};

}  // namespace IM::domain::service

#endif  // __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__
