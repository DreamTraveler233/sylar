#!/bin/bash

# 网关分离部署启动脚本

PROJECT_ROOT=$(pwd)
HTTP_BIN="${PROJECT_ROOT}/bin/im_gateway_http"
WS_BIN="${PROJECT_ROOT}/bin/im_gateway_ws"
PRESENCE_BIN="${PROJECT_ROOT}/bin/im_svc_presence"
MESSAGE_BIN="${PROJECT_ROOT}/bin/im_svc_message"
CONTACT_BIN="${PROJECT_ROOT}/bin/im_svc_contact"
USER_BIN="${PROJECT_ROOT}/bin/im_svc_user"
GROUP_BIN="${PROJECT_ROOT}/bin/im_svc_group"
TALK_BIN="${PROJECT_ROOT}/bin/im_svc_talk"
MEDIA_BIN="${PROJECT_ROOT}/bin/im_svc_media"
HTTP_CONF="${PROJECT_ROOT}/bin/config/gateway_http"
WS_CONF="${PROJECT_ROOT}/bin/config/gateway_ws"
WS2_CONF="${PROJECT_ROOT}/bin/config/gateway_ws_2"
PRESENCE_CONF="${PROJECT_ROOT}/bin/config/svc_presence"
MESSAGE_CONF="${PROJECT_ROOT}/bin/config/svc_message"
CONTACT_CONF="${PROJECT_ROOT}/bin/config/svc_contact"
USER_CONF="${PROJECT_ROOT}/bin/config/svc_user"
GROUP_CONF="${PROJECT_ROOT}/bin/config/svc_group"
TALK_CONF="${PROJECT_ROOT}/bin/config/svc_talk"
MEDIA_CONF="${PROJECT_ROOT}/bin/config/svc_media"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function start() {
    echo -e "${YELLOW}正在启动 Presence 服务...${NC}"
    "${PRESENCE_BIN}" -c "${PRESENCE_CONF}" -d

    echo -e "${YELLOW}正在启动 Contact 服务...${NC}"
    "${CONTACT_BIN}" -c "${CONTACT_CONF}" -d

    echo -e "${YELLOW}正在启动 User 服务...${NC}"
    "${USER_BIN}" -c "${USER_CONF}" -d

    echo -e "${YELLOW}正在启动 Message 服务...${NC}"
    "${MESSAGE_BIN}" -c "${MESSAGE_CONF}" -d

    echo -e "${YELLOW}正在启动 Group 服务...${NC}"
    "${GROUP_BIN}" -c "${GROUP_CONF}" -d

    echo -e "${YELLOW}正在启动 Talk 服务...${NC}"
    "${TALK_BIN}" -c "${TALK_CONF}" -d

    echo -e "${YELLOW}正在启动 Media 服务...${NC}"
    "${MEDIA_BIN}" -c "${MEDIA_CONF}" -d

    echo -e "${YELLOW}正在启动 HTTP 网关...${NC}"
    "${HTTP_BIN}" -c "${HTTP_CONF}" -d
    
    echo -e "${YELLOW}正在启动 WebSocket 网关...${NC}"
    "${WS_BIN}" -c "${WS_CONF}" -d

    echo -e "${YELLOW}正在启动 WebSocket 网关(实例2)...${NC}"
    "${WS_BIN}" -c "${WS2_CONF}" -d
    
    echo -e "${GREEN}网关进程已在后台启动${NC}"
}

function stop() {
    echo -e "${YELLOW}正在停止网关进程...${NC}"
    pkill -f "im_svc_presence -c ${PRESENCE_CONF}"
    pkill -f "im_svc_contact -c ${CONTACT_CONF}"
    pkill -f "im_svc_user -c ${USER_CONF}"
    pkill -f "im_svc_message -c ${MESSAGE_CONF}"
    pkill -f "im_svc_group -c ${GROUP_CONF}"
    pkill -f "im_svc_talk -c ${TALK_CONF}"
    pkill -f "im_svc_media -c ${MEDIA_CONF}"
    pkill -f "im_gateway_http -c ${HTTP_CONF}"
    pkill -f "im_gateway_ws -c ${WS_CONF}"
    pkill -f "im_gateway_ws -c ${WS2_CONF}"
    echo -e "${GREEN}信号已发送${NC}"
}

function status() {
    echo -e "${YELLOW}网关运行状态：${NC}"
    ps aux | grep -v grep | grep -E "im_svc_presence|im_svc_contact|im_svc_user|im_svc_message|im_svc_group|im_svc_talk|im_svc_media|im_gateway_http|im_gateway_ws"
    
    echo -e "\n${YELLOW}监听端口状态：${NC}"
    netstat -tunlp | grep -E "8080|8081|8082|8060|8061|8070|8071|8072|8073|8074|8075|8076"
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    status)
        status
        ;;
    restart)
        stop
        sleep 1
        start
        ;;
    *)
        echo "用法: $0 {start|stop|status|restart}"
        exit 1
esac
