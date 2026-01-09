#include "application/app/media_service_impl.hpp"

#include "core/base/macro.hpp"
#include "core/system/application.hpp"
#include "infra/db/mysql.hpp"
#include "infra/repository/media_repository_impl.hpp"
#include "infra/storage/istorage.hpp"
#include "interface/media/media_module.hpp"

/**
 * @brief Media 服务进程入口（阶段 4：svc_media）
 *
 * 责任：
 * - 媒体上传会话管理（分片）
 * - 合并分片并生成 MediaFile 记录
 * - 查询 MediaFile 信息（供 user/avatar 等业务使用）
 */
int main(int argc, char **argv)
{
    IM::Application app;
    if (!app.init(argc, argv))
    {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_media init failed";
        return 1;
    }

    std::srand(std::time(nullptr));

    auto db_manager =
        std::shared_ptr<IM::MySQLManager>(IM::MySQLMgr::GetInstance(), [](IM::MySQLManager *) {});

    auto media_repo = std::make_shared<IM::infra::repository::MediaRepositoryImpl>(db_manager);
    auto storage_adapter = IM::infra::storage::CreateLocalStorageAdapter();
    auto media_service = std::make_shared<IM::app::MediaServiceImpl>(media_repo, storage_adapter);

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::media::MediaModule>(media_service));

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_media is starting...";
    return app.run() ? 0 : 2;
}
