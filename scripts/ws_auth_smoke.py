#!/usr/bin/env python3
import argparse
import base64
import json
import os
import socket
import struct


def recv_exact(sock: socket.socket, n: int) -> bytes:
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise EOFError("socket closed")
        buf += chunk
    return buf


def recv_ws_frame(sock: socket.socket) -> tuple[int, bytes]:
    b1, b2 = recv_exact(sock, 2)
    opcode = b1 & 0x0F
    masked = (b2 >> 7) & 1
    ln = b2 & 0x7F

    if ln == 126:
        ln = struct.unpack("!H", recv_exact(sock, 2))[0]
    elif ln == 127:
        ln = struct.unpack("!Q", recv_exact(sock, 8))[0]

    mask = b""
    if masked:
        mask = recv_exact(sock, 4)

    payload = recv_exact(sock, ln) if ln else b""

    if masked:
        payload = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))

    return opcode, payload


def main() -> int:
    parser = argparse.ArgumentParser(description="WS auth smoke test for gateway_ws")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8081)
    parser.add_argument("--token", default=os.environ.get("IM_TOKEN", ""))
    parser.add_argument("--platform", default="web")
    args = parser.parse_args()

    if not args.token:
        raise SystemExit("missing --token (or IM_TOKEN env var)")

    path = f"/wss/default.io?token={args.token}&platform={args.platform}"
    key = base64.b64encode(os.urandom(16)).decode()

    req = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {args.host}:{args.port}\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n"
    )

    sock = socket.create_connection((args.host, args.port), timeout=5)
    sock.sendall(req.encode())

    resp = sock.recv(4096)
    head = resp.decode(errors="replace").split("\r\n\r\n", 1)[0]
    print(head)

    opcode, payload = recv_ws_frame(sock)
    text = payload.decode(errors="replace")
    print("opcode:", opcode)
    print("payload:", text)

    try:
        j = json.loads(text)
        print("event:", j.get("event"))
        print("uid:", (j.get("payload") or {}).get("uid"))
    except Exception as e:
        print("json parse failed:", e)

    sock.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
