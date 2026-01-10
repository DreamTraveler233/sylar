#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/db/mysql.hpp"
#include "infra/module/crypto_module.hpp"
#include "infra/repository/common_repository_impl.hpp"
#include "infra/repository/talk_repository_impl.hpp"
#include "infra/repository/user_repository_impl.hpp"

#include "application/app/common_service_impl.hpp"
#include "application/app/user_service_impl.hpp"
#include "application/rpc/media_service_rpc_client.hpp"

#include "interface/user/user_module.hpp"

/**
 * @brief User 服务进程入口（阶段4 - svc_user）
 *
 * 责任：
 * - 用户鉴权/注册/找回密码
 * - 用户资料/设置读写
 * - 在线状态（DB 标记）
 * - 登录日志写入
 */
int main(int argc, char **argv) {
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_user init failed";
        return 1;
    }

    std::srand(std::time(nullptr));

    // 加解密模块（DecryptPassword 等依赖）
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::CryptoModule>());

    auto db_manager = std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    auto user_repo = std::make_shared<IM::infra::repository::UserRepositoryImpl>(db_manager);
    auto talk_repo = std::make_shared<IM::infra::repository::TalkRepositoryImpl>(db_manager);
    auto common_repo = std::make_shared<IM::infra::repository::CommonRepositoryImpl>(db_manager);
    IM::domain::service::IMediaService::Ptr media_service = std::make_shared<IM::app::rpc::MediaServiceRpcClient>();
    auto common_service = std::make_shared<IM::app::CommonServiceImpl>(common_repo);

    auto user_service = std::make_shared<IM::app::UserServiceImpl>(user_repo, media_service, common_service, talk_repo);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::user::UserModule>(user_service, user_repo));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_user is starting...";
    return app.run() ? 0 : 2;
}
