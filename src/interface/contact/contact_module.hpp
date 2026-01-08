#ifndef __IM_CONTACT_CONTACT_MODULE_HPP__
#define __IM_CONTACT_CONTACT_MODULE_HPP__

#include "domain/service/contact_query_service.hpp"
#include "infra/module/module.hpp"

namespace IM::contact {

class ContactModule : public IM::RockModule {
   public:
    explicit ContactModule(IM::domain::service::IContactQueryService::Ptr contact_query_service);
    ~ContactModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    IM::domain::service::IContactQueryService::Ptr m_contact_query_service;
};

}  // namespace IM::contact

#endif  // __IM_CONTACT_CONTACT_MODULE_HPP__
