#!/usr/bin/env bash
# 사용법:
#   ./push.sh                 scripts/main.lua 전송 → 리로드
#   ./push.sh 파일.lua        지정 파일 전송
#   ./push.sh status          마지막 런타임 에러 + print 출력 조회
PORT=7777
exec 3<>/dev/tcp/127.0.0.1/$PORT || { echo "연결 실패: 앱 실행 중인지 확인"; exit 1; }
if [ "$1" != "status" ]; then
    cat "${1:-src/core/main.lua}" >&3 || exit 1
fi
printf '\0' >&3
cat <&3

