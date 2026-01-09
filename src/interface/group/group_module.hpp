#pragma once

#include "domain/service/group_service.hpp"
#include "infra/module/module.hpp"

namespace IM::group {

class GroupModule : public IM::RockModule {
public:
    explicit GroupModule(IM::domain::service::IGroupService::Ptr group_service);
    ~GroupModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

private:
    IM::domain::service::IGroupService::Ptr m_group_service;
};

} // namespace IM::group
