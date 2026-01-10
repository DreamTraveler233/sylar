#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/db/mysql.hpp"
#include "infra/module/crypto_module.hpp"
#include "infra/repository/message_repository_impl.hpp"
#include "infra/repository/talk_repository_impl.hpp"
#include "infra/repository/user_repository_impl.hpp"

#include "application/app/message_service_impl.hpp"
#include "application/rpc/contact_query_service_rpc_client.hpp"

#include "interface/message/message_module.hpp"

/**
 * @brief 消息服务进程入口（阶段3）
 *
 * 责任：
 * 1. 消息写入/查询
 * 2. 会话摘要更新
 * 3. 写入后触发推送（通过 WsGatewayModule 静态方法跨网关投递）
 */
int main(int argc, char **argv) {
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_message init failed";
        return 1;
    }

    std::srand(std::time(nullptr));

    // 加解密模块（部分业务可能依赖）
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::CryptoModule>());

    auto db_manager = std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    auto message_repo = std::make_shared<IM::infra::repository::MessageRepositoryImpl>(db_manager);
    auto talk_repo = std::make_shared<IM::infra::repository::TalkRepositoryImpl>(db_manager);
    auto user_repo = std::make_shared<IM::infra::repository::UserRepositoryImpl>(db_manager);
    auto contact_query_service = std::make_shared<IM::app::rpc::ContactQueryServiceRpcClient>();

    auto message_service =
        std::make_shared<IM::app::MessageServiceImpl>(message_repo, talk_repo, user_repo, contact_query_service);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::message::MessageModule>(message_service));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_message is starting...";
    return app.run() ? 0 : 2;
}
