/**
 * @file article_service_impl.hpp
 * @brief 应用层服务实现
 * @author DreamTraveler233
 * @date 2026-01-10
 *
 * 该文件是 XinYu-IM 项目的组成部分，主要负责 应用层服务实现。
 */

#pragma once
#include "domain/repository/article_repository.hpp"
#include "domain/service/article_service.hpp"

namespace IM::app {

class ArticleServiceImpl : public domain::service::IArticleService {
   public:
    ArticleServiceImpl(domain::repository::IArticleRepository::Ptr repo);

    // Classify
    Result<std::vector<dto::ArticleClassifyItem>> GetClassifyList(uint64_t user_id) override;
    Result<void> EditClassify(uint64_t user_id, uint64_t classify_id, const std::string &name) override;
    Result<void> DeleteClassify(uint64_t user_id, uint64_t classify_id) override;
    Result<void> SortClassify(uint64_t user_id, uint64_t classify_id, int sort_index) override;

    // Article
    Result<uint64_t> EditArticle(uint64_t user_id, uint64_t article_id, const std::string &title,
                                 const std::string &abstract, const std::string &content, const std::string &image,
                                 uint64_t classify_id, int status) override;
    Result<void> DeleteArticle(uint64_t user_id, uint64_t article_id) override;
    Result<void> ForeverDeleteArticle(uint64_t user_id, uint64_t article_id) override;
    Result<void> RecoverArticle(uint64_t user_id, uint64_t article_id) override;
    Result<dto::ArticleDetail> GetArticleDetail(uint64_t user_id, uint64_t article_id) override;
    Result<std::pair<std::vector<dto::ArticleItem>, int>> GetArticleList(uint64_t user_id, int page, int size,
                                                                         uint64_t classify_id,
                                                                         const std::string &keyword,
                                                                         int find_type) override;
    Result<void> MoveArticle(uint64_t user_id, uint64_t article_id, uint64_t classify_id) override;
    Result<void> SetArticleTags(uint64_t user_id, uint64_t article_id, const std::vector<std::string> &tags) override;
    Result<void> SetArticleAsterisk(uint64_t user_id, uint64_t article_id, int type) override;

    // Annex
    Result<void> UploadAnnex(uint64_t user_id, uint64_t article_id, const std::string &name, int64_t size,
                             const std::string &path, const std::string &mime) override;
    Result<void> DeleteAnnex(uint64_t user_id, uint64_t annex_id) override;
    Result<void> ForeverDeleteAnnex(uint64_t user_id, uint64_t annex_id) override;
    Result<void> RecoverAnnex(uint64_t user_id, uint64_t annex_id) override;
    Result<std::vector<dto::ArticleAnnexItem>> GetRecycleAnnexList(uint64_t user_id) override;

   private:
    domain::repository::IArticleRepository::Ptr repo_;
};

}  // namespace IM::app
