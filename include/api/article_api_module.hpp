#ifndef __CIM_API_ARTICLE_API_MODULE_HPP__
#define __CIM_API_ARTICLE_API_MODULE_HPP__

#include "other/module.hpp"

namespace CIM::api {

class ArticleApiModule : public CIM::Module {
   public:
    ArticleApiModule();
    ~ArticleApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace CIM::api

#endif // __CIM_API_ARTICLE_API_MODULE_HPP__