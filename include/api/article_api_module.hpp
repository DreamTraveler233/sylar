#ifndef __IM_API_ARTICLE_API_MODULE_HPP__
#define __IM_API_ARTICLE_API_MODULE_HPP__

#include "other/module.hpp"

namespace IM::api {

class ArticleApiModule : public IM::Module {
   public:
    ArticleApiModule();
    ~ArticleApiModule() override = default;

    bool onServerReady() override;
};

}  // namespace IM::api

#endif // __IM_API_ARTICLE_API_MODULE_HPP__