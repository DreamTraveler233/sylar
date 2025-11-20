#ifndef __CIM_DAO_USER_SETTINGS_DAO_HPP__
#define __CIM_DAO_USER_SETTINGS_DAO_HPP__

#include <cstdint>
#include <ctime>
#include <string>

namespace CIM::dao {

struct UserSettings {
    uint64_t user_id = 0;                     // 用户ID，外键关联users.id
    std::string theme_mode = "light";         // 主题模式：light/dark/auto
    std::string theme_bag_img = "";           // 主题背景图片URL
    std::string theme_color = "#409eff";      // 主题颜色，十六进制
    std::string notify_cue_tone = "N";        // 通知提示音: Y是 N否
    std::string keyboard_event_notify = "N";  // 键盘事件通知：Y是 N否
    std::time_t created_at = 0;               // 创建时间
    std::time_t updated_at = 0;               // 更新时间
};

struct ConfigInfo {
    std::string theme_mode;             // 主题模式：light亮色 dark暗色
    std::string theme_bag_img;          // 聊天背景图片
    std::string theme_color;            // 主题主色调，十六进制
    std::string notify_cue_tone;        // 通知提示音
    std::string keyboard_event_notify;  // 键盘事件通知：Y是 N否
};

class UserSettingsDAO {
   public:
    // 创建或更新用户设置
    static bool Upsert(const UserSettings& settings, std::string* err = nullptr);

    // 获取用户配置详情
    static bool GetConfigInfo(uint64_t user_id, ConfigInfo& out, std::string* err = nullptr);
};

}  // namespace CIM::dao

#endif // __CIM_DAO_USER_SETTINGS_DAO_HPP__