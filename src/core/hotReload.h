#pragma once
// Lua 핫 리로드 TCP 서버 (127.0.0.1:port, NUL 바이트로 끝). 데스크톱/안드로이드 공용.
//   빈 내용 → 상태 조회 (마지막 에러 + print 출력)
//   내용 있음 → 스크립트 리로드
struct App;

bool hotReloadOpen(App &a, int port = 7777); // 실패하면 false (앱은 계속 동작)
void hotReloadPoll(App &a);                  // 매 프레임 호출 (non-blocking)
void hotReloadClose(App &a);
