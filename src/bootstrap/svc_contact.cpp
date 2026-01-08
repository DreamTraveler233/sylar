#include "application/app/contact_query_service_impl.hpp"

#include "core/base/macro.hpp"
#include "core/system/application.hpp"
#include "infra/db/mysql.hpp"
#include "infra/repository/contact_repository_impl.hpp"
#include "interface/contact/contact_module.hpp"

/**
 * @brief Contact 服务进程入口（阶段4 - 最小可用）
 *
 * 责任：提供联系人查询类 RPC（例如好友关系/备注/头像等），供其它服务（如 svc_message）使用。
 */
int main(int argc, char **argv)
{
    IM::Application app;
    if (!app.init(argc, argv))
    {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_contact init failed";
        return 1;
    }

    auto db_manager =
        std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    auto contact_repo = std::make_shared<IM::infra::repository::ContactRepositoryImpl>(db_manager);
    auto contact_query_service = std::make_shared<IM::app::ContactQueryServiceImpl>(contact_repo);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::contact::ContactModule>(contact_query_service));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_contact is starting...";
    return app.run() ? 0 : 2;
}
