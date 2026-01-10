# scripts 目录说明文档

本目录包含了 XinYu-IM-Backend 项目的构建、环境管理、部署运行及测试脚本。我们采用了按功能分类的目录结构，以提升项目可维护性。

## 目录结构概览

```
scripts/
├── db/             # 数据库相关脚本（迁移、初始化）
├── deploy/         # 部署与进程管理脚本
├── docker/         # Docker 容器化方案与 Nginx 配置
├── env/            # 基础依赖环境（Redis, MySQL, MinIO 等）管理
├── tests/          # 自动化测试脚本（冒烟测试、压力测试）
└── build.sh        # 全局编译构建脚本
```

## 1. 编译构建

### [build.sh](build.sh)
- **功能**: 封装 CMake 编译流程。
- **输出**: 
  - 可执行文件: bin/ (如 gateway_http, gateway_ws, im_svc_media 等)
  - 库文件: lib/

## 2. 数据库管理 (db/)

### [db/migrate.sh](db/migrate.sh)
- **功能**: 执行 migrations/ 目录下的 SQL 迁移文件。
- **依赖**: 需要系统中安装有 mysql 客户端，并能访问当前配置的数据库。

## 3. 部署与运行 (deploy/)

### [deploy/gateways.sh](deploy/gateways.sh)
- **功能**: 管理分层架构下的网关与业务服务进程。
- **命令**:
  - ./deploy/gateways.sh start: 启动所有网关及微服务。
  - ./deploy/gateways.sh stop: 优雅停止进程。
  - ./deploy/gateways.sh status: 查看当前运行状态、PID 及监听端口。

## 4. 环境管理 (env/)

### [env/start_env.sh](env/start_env.sh) & [env/stop_env.sh](env/stop_env.sh)
- **功能**: 基于 Docker Compose 或本地包管理工具启动/停止 Redis, MySQL, MongoDB, MinIO 等基础设施。

## 5. 自动化测试 (tests/)

### [tests/smoke/api_smoke.sh](tests/smoke/api_smoke.sh)
- **功能**: 对 HTTP 网关进行全链路冒烟测试，包括用户注册、登录、好友申请、消息发送等。
- **测试报告**: 运行结束后输出各接口的响应状态。

### [tests/smoke/ws_smoke/](tests/smoke/ws_smoke/)
- **功能**: 基于 Node.js 的 WebSocket 并发连接与心跳测试。

### [tests/smoke/test_update_user_info.sh](tests/smoke/test_update_user_info.sh)
- **功能**: 验证用户信息更新逻辑及数据库字段绑定正确性。

## 6. 工具与配置 (docker/)
包含 Nginx 的反向代理配置 (nginx.dev.conf) 及快速启动 Nginx 容器的脚本。
