# WebSocket 冒烟测试 (ws_smoke/)

用于验证 WebSocket 网关的长连接、认证及推送功能。

## 脚本说明

### [ws_auth_smoke.py](ws_auth_smoke.py)
验证 WebSocket 握手认证流程。
- 模拟客户端建立连接。
- 发送包含 JWT 的认证协议包。
- 检查服务器是否正确返回登录成功响应。

### [ws_listen_smoke.py](ws_listen_smoke.py)
验证消息监听与实时下发。
- 保持长连接。
- 监听来自服务器的 `Notify` 消息。
- 验证 Trace ID 在 WebSocket 链路中的透传逻辑。

## 运行环境
- Python 3.x
- `websocket-client` 库 (`pip install websocket-client`)

## 用法示例
```bash
python3 ws_auth_smoke.py --host 127.0.0.1 --port 8085 --token <YOUR_JWT_TOKEN>
```
