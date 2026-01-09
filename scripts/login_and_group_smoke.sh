#!/usr/bin/env bash
set -euo pipefail

BASE=${BASE:-http://127.0.0.1:8080}
MOBILE=${MOBILE:-18800005555}
PASS=${PASS:-Abc12345}
PLATFORM=${PLATFORM:-web}
TOKEN_FILE=${TOKEN_FILE:-/tmp/im_token}
GROUP_FILE=${GROUP_FILE:-/tmp/im_group_id}

ENC_PASS=$(printf %s "$PASS" | openssl pkeyutl -encrypt -pubin -inkey bin/keys/rsa_public_rsa_2048.pem -pkeyopt rsa_padding_mode:pkcs1 | base64 -w0)

LOGIN_JSON=$(curl -sS --max-time 5 -X POST "$BASE/api/v1/auth/login" \
  -H 'Content-Type: application/json' \
  -d "{\"mobile\":\"$MOBILE\",\"password\":\"$ENC_PASS\",\"platform\":\"$PLATFORM\"}")

TOKEN=$(printf '%s' "$LOGIN_JSON" | python3 -c 'import json,sys; j=json.load(sys.stdin); print((j.get("access_token") or (j.get("data") or {}).get("access_token") or ""))')

if [[ -z "$TOKEN" ]]; then
  echo "login failed: $LOGIN_JSON" >&2
  exit 3
fi

echo -n "$TOKEN" > "$TOKEN_FILE"

echo "token_prefix=${TOKEN:0:16}"

GROUP_JSON=$(curl -sS --max-time 5 -X POST "$BASE/api/v1/group/create" \
  -H 'Content-Type: application/json' \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"name":"rpc_group","avatar":"","notice":"","member_ids":[]}' )

GROUP_ID=$(printf '%s' "$GROUP_JSON" | python3 -c 'import json,sys; j=json.load(sys.stdin);
d = j.get("data") if isinstance(j, dict) and isinstance(j.get("data"), dict) else (j if isinstance(j, dict) else {});
gid = d.get("group_id", "");
print(gid if gid is not None else "")')

if [[ -z "$GROUP_ID" ]]; then
  echo "group/create failed: $GROUP_JSON" >&2
  exit 4
fi

echo -n "$GROUP_ID" > "$GROUP_FILE"

echo "group_id=$GROUP_ID"
