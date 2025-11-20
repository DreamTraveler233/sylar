#ifndef __CIM_API_AUTH_API_MODULE_HPP__
#define __CIM_API_AUTH_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class AuthApiModule : public CIM::Module {
   public:
    AuthApiModule();
    ~AuthApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_AUTH_API_MODULE_HPP__
