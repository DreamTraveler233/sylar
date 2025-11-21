#ifndef __IM_API_AUTH_API_MODULE_HPP__
#define __IM_API_AUTH_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class AuthApiModule : public IM::Module {
   public:
    AuthApiModule();
    ~AuthApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_AUTH_API_MODULE_HPP__
