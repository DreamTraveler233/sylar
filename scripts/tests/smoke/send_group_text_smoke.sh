#!/usr/bin/env bash
set -euo pipefail

BASE=${BASE:-http://127.0.0.1:8080}
TOKEN_FILE=${TOKEN_FILE:-/tmp/im_token}
GROUP_FILE=${GROUP_FILE:-/tmp/im_group_id}
TEXT=${1:-ws_broadcast_check}

if [[ ! -f "$TOKEN_FILE" ]]; then
  echo "missing token file: $TOKEN_FILE" >&2
  exit 2
fi
if [[ ! -f "$GROUP_FILE" ]]; then
  echo "missing group_id file: $GROUP_FILE" >&2
  exit 2
fi

TOKEN=$(cat "$TOKEN_FILE")
GROUP_ID=$(cat "$GROUP_FILE")
MSG_ID=$(python3 - <<'PY'
import secrets
print(secrets.token_hex(16))
PY
)

curl -sS --max-time 5 -X POST "$BASE/api/v1/message/send" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $TOKEN" \
  -d "{\"msg_id\":\"$MSG_ID\",\"talk_mode\":2,\"to_from_id\":$GROUP_ID,\"type\":\"text\",\"body\":{\"text\":\"$TEXT\"}}" | cat

echo
echo "msg_id=$MSG_ID"
