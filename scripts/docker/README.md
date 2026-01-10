# Docker 与 代理配置 (docker/)

本目录包含用于本地开发调试的容器化配置和 Nginx 反向代理配置。

## Nginx 配置

项目使用 Nginx 作为统一入口，处理负载均衡、SSL 卸载以及动静分离。

### 核心配置文件
- **[nginx.dev.conf](nginx.dev.conf)**: 开发环境的主配置。
  - 监听 `80` 端口转发至 HTTP 网关。
  - 代理 `/ws` 路径至 WebSocket 网关。
- **[nginx.media.conf](nginx.media.conf)**: 媒体资源服务代理配置。
  - 处理 `/media` 路径的文件预览。
  - 限制最大上传文件大小 (`client_max_body_size`)。

### 辅助脚本
- **[nginx-dev.sh](nginx-dev.sh)**: 快速启动一个 Nginx 容器并挂载本地配置文件。
  ```bash
  ./scripts/docker/nginx-dev.sh start
  ```

## 运维说明
详细的 Nginx 部署说明请参考：**[nginx-dev.md](nginx-dev.md)**。

## 注意事项
- 在 Docker 模式下，Nginx 配置文件中的 `upstream` 地址需与主机或容器网络对齐（通常使用 `host.docker.internal` 或网桥 IP）。
