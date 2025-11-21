#ifndef __IM_API_TALK_API_MODULE_HPP__
#define __IM_API_TALK_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class TalkApiModule : public IM::Module {
   public:
    TalkApiModule();
    ~TalkApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_TALK_API_MODULE_HPP__