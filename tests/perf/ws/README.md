# WebSocket 性能测试（占位）

后续会在这里补：

- WS message QPS（每秒消息数）
- 端到端延迟（客户端发 -> 服务端处理 -> 服务端回）
- 连接数/并发场景下的 RSS/峰值内存

建议测试目标：`gateway_ws` 的 `/wss/default.io`，结合一个轻量的 echo/heartbeat 事件。
