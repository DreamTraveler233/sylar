# 部署与运维脚本 (deploy/)

本目录包含用于管理 XinYu-IM 服务进程和初始化生产环境的脚本。

## 核心脚本

### [gateways.sh](gateways.sh)
这是最常用的进程管理工具。它负责启动和停止所有网关（HTTP/WS）以及后端业务微服务。

**基本用法：**
- `scripts/deploy/gateways.sh start`: 启动所有服务进程（后台运行）。
- `scripts/deploy/gateways.sh stop`: 优雅停止所有关联的 `im_*` 进程。
- `scripts/deploy/gateways.sh restart`: 重新启动所有服务。
- `scripts/deploy/gateways.sh status`: 显示当前正在运行的服务列表、进程 ID 及监听的端口。

**日志位置：**
标准输出和错误输出通常会被重定向到 `bin/log/` 目录下对应的服务文件夹中（如 `bin/log/gateway_http/stdout.log`）。

### [setup_media.sh](setup_media.sh)
用于初始化媒体资源存储目录和权限。在首次部署或存储路径变更时运行。

**主要功能：**
- 创建分布式文件存储所需的本地目录。
- 设置适当的写入权限（针对运行 `im_svc_media` 的用户）。
- (可选) 配置 Nginx 上传缓存目录。

**用法示例：**
```bash
sudo ./scripts/deploy/setup_media.sh
```

## 注意事项

1. **环境依赖**：在运行 `gateways.sh start` 之前，请确保数据库、Redis 等基础环境已通过 `scripts/env/start_env.sh` 启动。
2. **配置文件**：脚本默认加载 `bin/config/` 下的 YAML 配置文件。如果修改了配置，建议执行 `restart`。
