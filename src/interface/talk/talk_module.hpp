#ifndef __IM_TALK_TALK_MODULE_HPP__
#define __IM_TALK_TALK_MODULE_HPP__

#include <memory>

#include "domain/service/talk_service.hpp"
#include "domain/repository/talk_repository.hpp"
#include "infra/module/module.hpp"

namespace IM::talk {

class TalkModule : public IM::RockModule {
   public:
    TalkModule(IM::domain::service::ITalkService::Ptr talk_service,
               IM::domain::repository::ITalkRepository::Ptr talk_repo);
    ~TalkModule() override = default;

    bool onServerUp() override;

    bool handleRockRequest(IM::RockRequest::ptr request, IM::RockResponse::ptr response,
                           IM::RockStream::ptr stream) override;

    bool handleRockNotify(IM::RockNotify::ptr notify, IM::RockStream::ptr stream) override;

   private:
    IM::domain::service::ITalkService::Ptr m_talk_service;
    IM::domain::repository::ITalkRepository::Ptr m_talk_repo;
};

}  // namespace IM::talk

#endif  // __IM_TALK_TALK_MODULE_HPP__
