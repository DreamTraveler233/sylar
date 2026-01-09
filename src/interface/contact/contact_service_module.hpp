#pragma once

#include "domain/service/contact_service.hpp"
#include "infra/module/module.hpp"

namespace IM::contact {

class ContactServiceModule : public IM::RockModule {
public:
    explicit ContactServiceModule(IM::domain::service::IContactService::Ptr contact_service);
    ~ContactServiceModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

private:
    IM::domain::service::IContactService::Ptr m_contact_service;
};

} // namespace IM::contact
