#ifndef __IM_APP_RPC_CONTACT_QUERY_SERVICE_RPC_CLIENT_HPP__
#define __IM_APP_RPC_CONTACT_QUERY_SERVICE_RPC_CLIENT_HPP__

#include <atomic>
#include <unordered_map>

#include <jsoncpp/json/json.h>

#include "core/io/lock.hpp"
#include "core/config/config.hpp"
#include "core/net/rock/rock_stream.hpp"
#include "domain/service/contact_query_service.hpp"

namespace IM::app::rpc
{

    class ContactQueryServiceRpcClient final : public IM::domain::service::IContactQueryService
    {
    public:
        ContactQueryServiceRpcClient();

        Result<IM::dto::ContactDetails> GetContactDetail(const uint64_t owner_id,
                                                         const uint64_t target_id) override;

    private:
        IM::RockResult::ptr rockJsonRequest(const std::string &ip_port, uint32_t cmd,
                                            const Json::Value &body, uint32_t timeout_ms);
        std::string resolveSvcContactAddr();

        bool parseContactDetails(const Json::Value &j, IM::dto::ContactDetails &out);

    private:
        IM::ConfigVar<std::string>::ptr m_rpc_addr;

        IM::RWMutex m_mutex;
        std::unordered_map<std::string, IM::RockConnection::ptr> m_conns;
        std::atomic<uint32_t> m_sn{1};
    };

} // namespace IM::app::rpc

#endif // __IM_APP_RPC_CONTACT_QUERY_SERVICE_RPC_CLIENT_HPP__
