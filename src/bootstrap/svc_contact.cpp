#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/db/mysql.hpp"
#include "infra/repository/contact_repository_impl.hpp"
#include "infra/repository/group_repository_impl.hpp"
#include "infra/repository/message_repository_impl.hpp"
#include "infra/repository/talk_repository_impl.hpp"

#include "application/app/contact_query_service_impl.hpp"
#include "application/app/contact_service_impl.hpp"
#include "application/app/talk_service_impl.hpp"
#include "application/rpc/message_service_rpc_client.hpp"
#include "application/rpc/user_service_rpc_client.hpp"

#include "interface/contact/contact_module.hpp"
#include "interface/contact/contact_service_module.hpp"

/**
 * @brief Contact 服务进程入口（阶段4 - svc_contact）
 *
 * 责任：提供联系人领域 Rock RPC（查询 + 完整业务），供网关/其它服务调用。
 */
int main(int argc, char **argv) {
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_contact init failed";
        return 1;
    }

    auto db_manager = std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    auto contact_repo = std::make_shared<IM::infra::repository::ContactRepositoryImpl>(db_manager);
    auto talk_repo = std::make_shared<IM::infra::repository::TalkRepositoryImpl>(db_manager);
    auto message_repo = std::make_shared<IM::infra::repository::MessageRepositoryImpl>(db_manager);
    auto group_repo = std::make_shared<IM::infra::repository::GroupRepositoryImpl>();

    // Query service (cmd=401)
    auto contact_query_service = std::make_shared<IM::app::ContactQueryServiceImpl>(contact_repo);

    // Cross-domain services
    IM::domain::service::IUserService::Ptr user_service = std::make_shared<IM::app::rpc::UserServiceRpcClient>();
    IM::domain::service::IMessageService::Ptr message_service =
        std::make_shared<IM::app::rpc::MessageServiceRpcClient>();

    // Local talk service
    auto talk_service = std::make_shared<IM::app::TalkServiceImpl>(talk_repo, contact_repo, message_repo, group_repo);

    // Full contact service (cmd=402-413)
    auto contact_service = std::make_shared<IM::app::ContactServiceImpl>(contact_repo, user_service, talk_repo,
                                                                         message_service, talk_service);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::contact::ContactModule>(contact_query_service));
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::contact::ContactServiceModule>(contact_service));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_contact is starting...";
    return app.run() ? 0 : 2;
}
