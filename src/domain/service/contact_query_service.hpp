#ifndef __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__
#define __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__

#include <memory>

#include "common/result.hpp"
#include "dto/contact_dto.hpp"

namespace IM::domain::service {

// 仅用于跨域读取联系人的“查询”能力（避免把完整 IContactService 引入到其它服务）。
class IContactQueryService {
   public:
    using Ptr = std::shared_ptr<IContactQueryService>;
    virtual ~IContactQueryService() = default;

    virtual Result<IM::dto::ContactDetails> GetContactDetail(const uint64_t owner_id,
                                                            const uint64_t target_id) = 0;
};

}  // namespace IM::domain::service

#endif  // __IM_DOMAIN_SERVICE_CONTACT_QUERY_SERVICE_HPP__
