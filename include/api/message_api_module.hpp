#ifndef __CIM_API_MESSAGE_API_MODULE_HPP__
#define __CIM_API_MESSAGE_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class MessageApiModule : public CIM::Module {
   public:
    MessageApiModule();
    ~MessageApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_MESSAGE_API_MODULE_HPP__