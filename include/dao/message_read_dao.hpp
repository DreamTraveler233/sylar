#ifndef __IM_DAO_MESSAGE_READ_DAO_HPP__
#define __IM_DAO_MESSAGE_READ_DAO_HPP__

#include <cstdint>
#include <memory>
#include <string>

namespace IM::dao {
// im_message_read 表 DAO：记录用户对消息的阅读（幂等插入）
class MessageReadDao {
   public:
    static bool MarkRead(const std::string& msg_id, const uint64_t user_id, std::string* err = nullptr);
    static bool MarkReadByTalk(const uint64_t talk_id, const uint64_t user_id, std::string* err = nullptr);
};
}  // namespace IM::dao

#endif  // __IM_DAO_MESSAGE_READ_DAO_HPP__
