#include "application/app/contact_query_service_impl.hpp"

namespace IM::app {

Result<IM::dto::ContactDetails> ContactQueryServiceImpl::GetContactDetail(const uint64_t owner_id,
                                                                          const uint64_t target_id) {
    Result<IM::dto::ContactDetails> result;

    IM::dto::ContactDetails out;
    std::string err;
    if (!m_contact_repo || !m_contact_repo->GetByOwnerAndTarget(owner_id, target_id, out, &err)) {
        result.code = 500;
        result.err = err.empty() ? "get contact detail failed" : err;
        return result;
    }

    result.data = std::move(out);
    result.ok = true;
    return result;
}

}  // namespace IM::app
