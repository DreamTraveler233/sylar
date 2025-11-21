#ifndef __IM_API_USER_API_MODULE_HPP__
#define __IM_API_USER_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class UserApiModule : public IM::Module {
   public:
    UserApiModule();
    ~UserApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_USER_API_MODULE_HPP__