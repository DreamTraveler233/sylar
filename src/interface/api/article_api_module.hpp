/**
 * @file article_api_module.hpp
 * @brief 接口定义与模块实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 接口定义与模块实现。
 */

#ifndef __IM_API_ARTICLE_API_MODULE_HPP__
#define __IM_API_ARTICLE_API_MODULE_HPP__

#include "infra/module/module.hpp"

#include "domain/service/article_service.hpp"

namespace IM::api {

class ArticleApiModule : public IM::Module {
   public:
    ArticleApiModule();
    ~ArticleApiModule() override = default;

    bool onServerReady() override;

   private:
    IM::domain::service::IArticleService::Ptr m_article_service;
};

}  // namespace IM::api

#endif  // __IM_API_ARTICLE_API_MODULE_HPP__