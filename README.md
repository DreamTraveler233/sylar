# XinYu IM Backend

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)]()
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)]()

XinYu IM Backend 是一个基于 **C++17** 开发的高性能即时通讯服务端。项目采用分层架构设计，实现了 **网关分离** 与 **分布式链路追踪**，能够在保证高并发性能的同时，提供极佳的可维护性与可观测性。

## ✨ 特性

- 🚀 **高性能并发模型**：基于 Libevent + 协程（Coroutine）的非阻塞 IO 模型，轻松应对动态长连接压力的同时保持同步代码的编写体验。
- 🏗️ **网关分离架构**：
  - **HTTP 网关**: 负责 RESTful API、身份鉴权及业务逻辑触发。
  - **WebSocket 网关**: 负责长连接维护、心跳检测及实时消息下发。
- 🔗 **分布式链路追踪**：内置协程安全的 **Trace ID** 追踪机制，支持跨 RPC 节点的日志染色，实现全链路请求定位。
- 🛰️ **服务发现与自治**：集成 ZooKeeper 动态注册与发现服务节点，支持水平扩容与故障感知。
- 📦 **微服务化演进**：已实现媒体服务 (`im_svc_media`) 的解耦拆分，支持 Rock RPC 跨进程通信。

## 🚀 快速开始

### 先决条件
- **操作系统**: Linux (推荐 Ubuntu 20.04+)
- **编译器**: GCC 9+ 或 Clang 11+
- **依赖库**: Protobuf, OpenSSL, Zlib, YAML-cpp, JWT-cpp, MySQL Client, Hiredis, Libevent, ZooKeeper

### 安装步骤
```bash
# 1. 克隆项目
git clone https://github.com/szy/XinYu-IM-Backend.git
cd XinYu-IM-Backend

# 2. 启动基础依赖环境 (Redis, MySQL, ZooKeeper 等)
./scripts/env/start_env.sh

# 3. 执行数据库迁移
./scripts/db/migrate.sh apply

# 4. 编译项目
./scripts/build.sh
```

### 运行服务
```bash
# 一键启动 HTTP 与 WebSocket 所有网关服务
./scripts/deploy/gateways.sh start

# 检查运行状态
./scripts/deploy/gateways.sh status
```

## 📂 项目结构

```
XinYu-IM-Backend/
├── bin/                # 编译产物、配置文件及日志
│   ├── config/         # 业务配置文件 (YAML)
│   └── log/            # 服务运行日志
├── scripts/            # 自动化运维工具链
│   ├── db/             # 数据库迁移脚本
│   ├── deploy/         # 进程管理与部署
│   ├── env/            # 基础设施管理
│   └── tests/          # 冒烟测试与压测
├── src/                # 核心源代码
│   ├── core/           # 框架底层 (IO, 协程, 网络, 追踪)
│   ├── interface/      # 接口层 (HTTP/WS/RPC)
│   └── infra/          # 基础设施适配 (MySQL/Redis/Storage)
└── migrations/         # 数据库版本迁移 SQL
```

## 📖 文档
- [分布式项目计划书](docs/分布式IM服务器项目计划书.md)
- [服务发现约定](docs/service_discovery_convention.md)
- [脚本目录使用说明](scripts/README.md)

## 🤝 贡献
欢迎提交 Issue 或 PR。在提交代码前请确保：
1. 请运行 `.clang-format` 格式化代码。
2. 运行 `./scripts/tests/smoke/api_smoke.sh` 确保核心流程通过。

## 📄 许可证
本项目基于 [MIT License](LICENSE) 开源。

## 🙏 致谢
- [sylar](https://github.com/sylar-yin/sylar) - 分布式高性能服务器框架