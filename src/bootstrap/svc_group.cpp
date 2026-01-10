#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/db/mysql.hpp"
#include "infra/repository/contact_repository_impl.hpp"
#include "infra/repository/group_repository_impl.hpp"
#include "infra/repository/message_repository_impl.hpp"
#include "infra/repository/talk_repository_impl.hpp"

#include "application/app/group_service_impl.hpp"
#include "application/app/talk_service_impl.hpp"
#include "application/rpc/message_service_rpc_client.hpp"
#include "application/rpc/user_service_rpc_client.hpp"

#include "interface/group/group_module.hpp"

/**
 * @brief Group 服务进程入口（阶段4 - svc_group）
 *
 * 责任：提供群组领域的 Rock RPC 服务，供网关调用。
 */
int main(int argc, char **argv) {
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_group init failed";
        return 1;
    }

    auto db_manager = std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    // Repositories (local)
    auto talk_repo = std::make_shared<IM::infra::repository::TalkRepositoryImpl>(db_manager);
    auto message_repo = std::make_shared<IM::infra::repository::MessageRepositoryImpl>(db_manager);
    auto contact_repo = std::make_shared<IM::infra::repository::ContactRepositoryImpl>(db_manager);
    auto group_repo = std::make_shared<IM::infra::repository::GroupRepositoryImpl>();

    // Cross-domain services
    IM::domain::service::IUserService::Ptr user_service = std::make_shared<IM::app::rpc::UserServiceRpcClient>();
    IM::domain::service::IMessageService::Ptr message_service =
        std::make_shared<IM::app::rpc::MessageServiceRpcClient>();

    // Talk service (local)
    auto talk_service = std::make_shared<IM::app::TalkServiceImpl>(talk_repo, contact_repo, message_repo, group_repo);

    auto group_service =
        std::make_shared<IM::app::GroupServiceImpl>(group_repo, user_service, message_service, talk_service);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::group::GroupModule>(group_service));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_group is starting...";
    return app.run() ? 0 : 2;
}
