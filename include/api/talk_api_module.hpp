#ifndef __CIM_API_TALK_API_MODULE_HPP__
#define __CIM_API_TALK_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class TalkApiModule : public CIM::Module {
   public:
    TalkApiModule();
    ~TalkApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_TALK_API_MODULE_HPP__