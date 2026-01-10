/**
 * @file contact_query_service_impl.hpp
 * @brief 应用层服务实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 应用层服务实现。
 */

#ifndef __IM_APP_CONTACT_QUERY_SERVICE_IMPL_HPP__
#define __IM_APP_CONTACT_QUERY_SERVICE_IMPL_HPP__

#include "domain/repository/contact_repository.hpp"
#include "domain/service/contact_query_service.hpp"

namespace IM::app {

class ContactQueryServiceImpl : public IM::domain::service::IContactQueryService {
   public:
    explicit ContactQueryServiceImpl(IM::domain::repository::IContactRepository::Ptr contact_repo)
        : m_contact_repo(std::move(contact_repo)) {}

    Result<IM::dto::ContactDetails> GetContactDetail(const uint64_t owner_id, const uint64_t target_id) override;

   private:
    IM::domain::repository::IContactRepository::Ptr m_contact_repo;
};

}  // namespace IM::app

#endif  // __IM_APP_CONTACT_QUERY_SERVICE_IMPL_HPP__
