#include "core/base/macro.hpp"
#include "core/system/application.hpp"

#include "infra/module/crypto_module.hpp"
#include "infra/module/module.hpp"

#include "application/rpc/talk_repository_rpc_client.hpp"
#include "application/rpc/user_service_rpc_client.hpp"

#include "interface/api/ws_gateway_module.hpp"

/**
 * @brief WebSocket 网关进程入口
 *
 * 责任：
 * 1. 维护客户端 WebSocket 长连接
 * 2. 处理心跳、鉴权握手
 * 3. 接收下行推送并转发给客户端
 */
int main(int argc, char **argv) {
    /* 创建并初始化应用程序实例 */
    IM::Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "Gateway WS init failed";
        return 1;
    }

    std::srand(std::time(nullptr));

    // 注册加解密模块
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::CryptoModule>());

    // Talk repo (RPC): used for group broadcast member lookup
    auto talk_repo = std::make_shared<IM::app::rpc::TalkRepositoryRpcClient>();

    IM::domain::service::IUserService::Ptr user_service = std::make_shared<IM::app::rpc::UserServiceRpcClient>();

    // WebSocket 网关模块 (核心)
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::WsGatewayModule>(user_service, talk_repo));

    IM_LOG_INFO(IM_LOG_ROOT()) << "Gateway WS is starting...";
    return app.run() ? 0 : 2;
}
