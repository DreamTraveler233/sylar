/**
 * @file contact_service_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#pragma once

#include "infra/module/module.hpp"

#include "domain/service/contact_service.hpp"

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

}  // namespace IM::contact
