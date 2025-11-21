#ifndef __IM_APP_RESULT_HPP__
#define __IM_APP_RESULT_HPP__

#include "dao/contact_apply_dao.hpp"
#include "dao/contact_dao.hpp"
#include "dao/contact_group_dao.hpp"
#include "dao/message_dao.hpp"
#include "dao/sms_code_dao.hpp"
#include "dao/talk_session_dao.hpp"
#include "dao/user_dao.hpp"
#include "dao/user_login_log_dao.hpp"
#include "dao/user_settings_dao.hpp"
#include "util/id_worker.hpp"

namespace IM::app {

template <typename T>
struct Result {
    bool ok = false;
    int code = 0;     // 可选：错误码
    std::string err;  // 错误描述
    T data;           // 成功时的载荷
};

using VoidResult = Result<int>;  // data 不用时可用占位
using StatusResult = Result<std::string>;
using ApplyCountResult = Result<uint64_t>;
using UserResult = Result<IM::dao::User>;
using SmsCodeResult = Result<IM::dao::SmsCode>;
using UserInfoResult = Result<IM::dao::UserInfo>;
using ConfigInfoResult = Result<IM::dao::ConfigInfo>;
using ContactGroupResult = Result<IM::dao::ContactGroup>;
using TalkSessionResult = Result<IM::dao::TalkSessionItem>;
using ContactDetailsResult = Result<IM::dao::ContactDetails>;
using MessageRecordPageResult = Result<IM::dao::MessagePage>;
using MessageRecordResult = Result<IM::dao::MessageRecord>; // 单条消息结果（发送返回）
using ContactListResult = Result<std::vector<IM::dao::ContactItem>>;
using MessageRecordListResult = Result<std::vector<IM::dao::MessageRecord>>;
using TalkSessionListResult = Result<std::vector<IM::dao::TalkSessionItem>>;
using ContactApplyListResult = Result<std::vector<IM::dao::ContactApplyItem>>;
using ContactGroupListResult = Result<std::vector<IM::dao::ContactGroupItem>>;

}  // namespace IM::app

#endif  // __IM_APP_RESULT_HPP__