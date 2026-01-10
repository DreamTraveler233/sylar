#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/module/module.hpp"

#include "interface/presence/presence_module.hpp"

/**
 * @brief Presence 服务进程入口
 *
 * 责任：
 * 1. 维护全局在线路由（uid -> gateway_ws_rpc 地址）
 * 2. 为网关与后端服务提供查询接口（Rock RPC）
 */
int main(int argc, char **argv) {
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "svc_presence init failed";
        return 1;
    }

    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::presence::PresenceModule>());

    IM_LOG_INFO(IM_LOG_ROOT()) << "svc_presence is starting...";
    return app.run() ? 0 : 2;
}
