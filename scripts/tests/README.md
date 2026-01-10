# 自动化测试脚本 (tests/)

本目录包含 XinYu-IM 项目的各级测试工具，涵盖冒烟测试、性能压测及组件测试。

## 目录结构

### 1. [smoke/](smoke/) (冒烟测试)
用于快速验证系统核心功能是否正常，建议在每次代码提交或部署后运行。

- **[api_smoke.sh](smoke/api_smoke.sh)**: 全链路核心功能测试（PHP/Bash 驱动）。验证：
  - 用户注册与加密登录 (RSA + JWT)
  - 好友申请、接受与私聊
  - 群组创建、加入与群聊
- **[ws_smoke/](smoke/ws_smoke/)**: WebSocket 专项冒烟测试（Python 驱动）。验证：
  - 断线重连
  - 心跳机制
  - 实时消息推送
- **[test_update_user_info.sh](smoke/test_update_user_info.sh)**: 针对用户信息更新接口的专项回归测试。

### 2. [bench/](bench/) (性能测试)
包含针对高并发场景的压力测试工具。
- 基于 `wrk` 或 `locust` 的 HTTP 压测脚本。
- 模拟上万 WebSocket 连接并发的消息收发测试。

## 如何运行

### 运行完整冒烟测试
```bash
# 确保环境已启动且服务运行中
./scripts/tests/smoke/api_smoke.sh
```

### 运行 WebSocket 测试
```bash
cd scripts/tests/smoke/ws_smoke
python3 ws_auth_smoke.py
```

## 注意事项
- 测试脚本通常会向数据库写入测试数据。建议在 **开发/测试环境** 运行。
- 大部分脚本依赖 `curl`, `jq`, `openssl` 以及 `python3` 的相关库。
