#ifndef __IM_API_CONTACT_API_MODULE_HPP__
#define __IM_API_CONTACT_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class ContactApiModule : public IM::Module {
   public:
    ContactApiModule();
    ~ContactApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_CONTACT_API_MODULE_HPP__