/**
 * @file common_repository_impl.hpp
 * @brief 仓库模式实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 仓库模式实现。
 */

#ifndef __IM_INFRA_REPOSITORY_COMMON_REPOSITORY_IMPL_HPP__
#define __IM_INFRA_REPOSITORY_COMMON_REPOSITORY_IMPL_HPP__

#include "infra/db/mysql.hpp"

#include "domain/repository/common_repository.hpp"

namespace IM::infra::repository {

class CommonRepositoryImpl : public IM::domain::repository::ICommonRepository {
   public:
    explicit CommonRepositoryImpl(std::shared_ptr<IM::MySQLManager> db_manager);

    bool CreateEmailCode(const model::EmailVerifyCode &code, std::string *err = nullptr) override;
    bool VerifyEmailCode(const std::string &email, const std::string &code, const std::string &channel,
                         std::string *err = nullptr) override;
    bool MarkEmailCodeAsUsed(const uint64_t id, std::string *err = nullptr) override;
    bool MarkEmailCodeExpiredAsInvalid(std::string *err = nullptr) override;
    bool DeleteInvalidEmailCode(std::string *err = nullptr) override;
    bool CreateSmsCode(const model::SmsVerifyCode &code, std::string *err = nullptr) override;
    bool VerifySmsCode(const std::string &mobile, const std::string &code, const std::string &channel,
                       std::string *err = nullptr) override;
    bool MarkSmsCodeAsUsed(const uint64_t id, std::string *err = nullptr) override;
    bool MarkSmsCodeExpiredAsInvalid(std::string *err = nullptr) override;
    bool DeleteInvalidSmsCode(std::string *err = nullptr) override;

   private:
    std::shared_ptr<IM::MySQLManager> m_db_manager;
};

}  // namespace IM::infra::repository

#endif  // __IM_INFRA_REPOSITORY_COMMON_REPOSITORY_IMPL_HPP__