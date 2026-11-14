#pragma once
// 플랫폼 공용 GL 리소스 + 프레임 로직.
// 각 플랫폼 main.cpp 는 창/컨텍스트 생성, 이벤트 폴링, 버퍼 스왑만 담당.
struct App;

// 컨텍스트가 current 이고 GL 함수 로드가 끝난 뒤 호출
void appInitGL(App &a);
// 컨텍스트가 아직 current 일 때 호출 (텍스트 → 그리드 → 화살표 → 쿼드 → 셰이더)
void appTermGL(App &a);
// 렌더 타깃(기본 FB 또는 FBO)은 호출 쪽에서 바인드. w/h = 프레임버퍼 픽셀
void appFrame(App &a, float dt, int w, int h);
