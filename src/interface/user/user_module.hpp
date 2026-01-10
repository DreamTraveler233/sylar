/**
 * @file user_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_USER_USER_MODULE_HPP__
#define __IM_USER_USER_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/repository/user_repository.hpp"
#include "domain/service/user_service.hpp"

namespace IM::user {

class UserModule : public IM::RockModule {
   public:
    UserModule(IM::domain::service::IUserService::Ptr user_service,
               IM::domain::repository::IUserRepository::Ptr user_repo);
    ~UserModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    IM::domain::service::IUserService::Ptr m_user_service;
    IM::domain::repository::IUserRepository::Ptr m_user_repo;
};

}  // namespace IM::user

#endif  // __IM_USER_USER_MODULE_HPP__
