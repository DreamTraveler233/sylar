#ifndef __IM_API_GROUP_API_MODULE_HPP__
#define __IM_API_GROUP_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class GroupApiModule : public IM::Module {
   public:
    GroupApiModule();
    ~GroupApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_GROUP_API_MODULE_HPP__