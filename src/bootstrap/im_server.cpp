#include "api/article_api_module.hpp"
#include "api/auth_api_module.hpp"
#include "api/common_api_module.hpp"
#include "api/contact_api_module.hpp"
#include "api/emoticon_api_module.hpp"
#include "api/group_api_module.hpp"
#include "api/message_api_module.hpp"
#include "api/organize_api_module.hpp"
#include "api/talk_api_module.hpp"
#include "api/user_api_module.hpp"
#include "api/ws_gateway_module.hpp"
#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "other/crypto_module.hpp"
#include "other/module.hpp"
#include "system/application.hpp"

using namespace IM;

int main(int argc, char** argv) {
    /* 创建并初始化应用程序实例 */
    Application app;
    if (!app.init(argc, argv)) {
        IM_LOG_ERROR(IM_LOG_ROOT()) << "Application init failed";
        return 1;
    }

    std::srand(std::time(nullptr));  // 初始化随机数种子

    // 注册加解密模块
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::CryptoModule>());
    // 注册登录鉴权模块
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::AuthApiModule>());
    // 注册通用 API 模块
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::CommonApiModule>());

    /* 注册最小化 API 模块 */
    // 文章模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::ArticleApiModule>());
    // 联系人模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::ContactApiModule>());
    // 表情模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::EmoticonApiModule>());
    // 群组模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::GroupApiModule>());
    // 消息模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::MessageApiModule>());
    // 组织架构模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::OrganizeApiModule>());
    // 会话模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::TalkApiModule>());
    // 用户模块占位
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::UserApiModule>());
    // WebSocket 网关模块
    IM::ModuleMgr::GetInstance()->add(std::make_shared<IM::api::WsGatewayModule>());

    return app.run() ? 0 : 2;
}
