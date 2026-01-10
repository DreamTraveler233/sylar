/**
 * @file contact_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_CONTACT_CONTACT_MODULE_HPP__
#define __IM_CONTACT_CONTACT_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/service/contact_query_service.hpp"

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
