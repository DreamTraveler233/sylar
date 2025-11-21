#ifndef __IM_API_MESSAGE_API_MODULE_HPP__
#define __IM_API_MESSAGE_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class MessageApiModule : public IM::Module {
   public:
    MessageApiModule();
    ~MessageApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_MESSAGE_API_MODULE_HPP__