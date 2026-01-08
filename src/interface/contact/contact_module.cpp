#include "interface/contact/contact_module.hpp"

#include <jsoncpp/json/json.h>

#include "core/base/macro.hpp"
#include "core/util/json_util.hpp"

namespace IM::contact {

static auto g_logger = IM_LOG_NAME("root");

namespace {
constexpr uint32_t kCmdGetContactDetail = 401;

Json::Value ContactDetailsToJson(const IM::dto::ContactDetails& d) {
    Json::Value out(Json::objectValue);
    out["user_id"] = (Json::UInt64)d.user_id;
    out["avatar"] = d.avatar;
    out["gender"] = (Json::UInt)d.gender;
    out["mobile"] = d.mobile;
    out["motto"] = d.motto;
    out["nickname"] = d.nickname;
    out["email"] = d.email;
    out["relation"] = (Json::UInt)d.relation;
    out["contact_group_id"] = (Json::UInt)d.contact_group_id;
    out["contact_remark"] = d.contact_remark;
    return out;
}
}  // namespace

ContactModule::ContactModule(IM::domain::service::IContactQueryService::Ptr contact_query_service)
    : RockModule("svc.contact", "0.1.0", "builtin"),
      m_contact_query_service(std::move(contact_query_service)) {}

bool ContactModule::onServerUp() {
    registerService("rock", "im", "svc-contact");
    return true;
}

bool ContactModule::handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                                      IM::RockStream::ptr /*stream*/) {
    const auto cmd = request ? request->getCmd() : 0;
    if (cmd != kCmdGetContactDetail) {
        return false;
    }

    Json::Value body;
    if (!IM::JsonUtil::FromString(body, request->getBody()) || !body.isObject()) {
        response->setResult(400);
        response->setResultStr("invalid json body");
        return true;
    }

    const uint64_t owner_id = IM::JsonUtil::GetUint64(body, "owner_id");
    const uint64_t target_id = IM::JsonUtil::GetUint64(body, "target_id");

    if (owner_id == 0 || target_id == 0) {
        response->setResult(400);
        response->setResultStr("missing owner_id/target_id");
        return true;
    }

    if (!m_contact_query_service) {
        response->setResult(500);
        response->setResultStr("contact service not ready");
        return true;
    }

    auto r = m_contact_query_service->GetContactDetail(owner_id, target_id);
    if (!r.ok) {
        response->setResult(r.code == 0 ? 500 : r.code);
        response->setResultStr(r.err);
        return true;
    }

    Json::Value out(Json::objectValue);
    out["code"] = 200;
    out["data"] = ContactDetailsToJson(r.data);

    response->setBody(IM::JsonUtil::ToString(out));
    response->setResult(200);
    response->setResultStr("ok");
    return true;
}

bool ContactModule::handleRockNotify(IM::RockNotify::ptr /*notify*/, IM::RockStream::ptr /*stream*/) {
    return false;
}

}  // namespace IM::contact
