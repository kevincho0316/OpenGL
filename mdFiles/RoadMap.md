# 게임 엔진 로드맵 & 설계 구조


## 1. 현재 상태

**잘 되어 있는 것**
- `Renderer` + `DrawCmd` + `Pass` 정렬 (submit → flush 구조)
- `ShaderLib`: 경로를 기억했다가 컨텍스트 손실 후 `recreateAll()`
- `UniformValue` variant, `readFile()` 추상화
- Lua(sol2) 핫 리로드 + 소켓 push, 실패 시 이전 스크립트 유지
- Camera, Grid, Arrow2D, 텍스트 렌더링, 터치 입력

**가장 큰 문제: core가 Android 전용이 됨**
- `core/App.h`, `core/luaScript.cpp`, `core/input.h`가 `<EGL/egl.h>`, `<android/...>`, `android_native_app_glue.h`를 include
- `core/Rigidbody.h`, `core/arrow2D.h`가 `gl.h` 대신 `<GLES3/gl3.h>` 직접 include
- `platform/desktop/main.cpp`는 자기만의 `App` 구조체 + 옛날 방식(Lua에서 바로 `glDrawArrays`)을 씀 → Renderer/ShaderLib/Grid/텍스트 미사용
- Lua 바인딩, 소켓 서버 코드가 두 main.cpp에 중복

**작은 버그/정리거리**
- `Rigidbody.h`: `set_vel`/`set_accel`이 `glm::vec3` 반환형인데 return 없음 (UB) → `void`로. `#pragma once` 없음 (`object.h`도)
- `luaScript.cpp`의 `rect`: `s`가 nullptr일 때 `s->shader.ID` 접근
- `xmake.lua`: `add_packages('freetype')`인데 `add_requires`에 없음. 첫 줄 주석이 아직 `--sdk`
- `onInput`: MOVE마다 LOGI, 포인터 0번만 처리 (멀티터치 불가)
- `DEFAULT_SCRIPT`가 `main.lua` 내용을 복사해 둔 것 → 둘이 어긋남
- `DrawCmd`: 매 draw마다 `std::vector<ShaderUniform>`(문자열 이름) 할당, `glDrawArrays`만 지원(EBO 없음), 텍스처 슬롯 없음

## 2. 목표 구조

```
src/
├── gl.h
├── engine/
│   ├── core/       log, file, time, handle(id 타입)
│   ├── gfx/        Shader, Mesh, Texture + 컨텍스트 손실 복구   ← GL 호출은 여기만
│   ├── render/     Renderer, DrawCmd, Material, Camera
│   ├── scene/      World(엔티티), Transform, 컴포넌트, 시스템
│   ├── input/      InputState (포인터 여러 개, 키, 액션)
│   ├── script/     Lua 바인딩 (한 곳에만)
│   ├── debug/      Grid, Arrow2D, 텍스트, 핫리로드 소켓
│   └── Engine.h    엔진 진입 인터페이스
├── platform/
│   ├── desktop/    GLFW 창/입력 → Engine 호출만
│   └── android/    android_main/EGL → Engine 호출만
└── game/           main.lua, 게임 전용 코드
```

**의존 방향 (아래로만)**
`platform → Engine → script → scene → render → gfx → core`

**Engine 인터페이스 (플랫폼이 부르는 전부)**
- `onContextCreated()` / `onContextLost()` : GL 리소스 생성/해제
- `onInput(event)` : 플랫폼 중립 이벤트
- `update(dt)` / `render(w, h)`
- 플랫폼 계층은 EGL/GLFW 핸들을 자기가 갖고, Engine은 그걸 모름

**핵심 설계 결정**
1. **리소스는 핸들(id)로**: 게임 코드는 `MeshId`, `TextureId`, `ShaderId`만 들고, 실제 GLuint는 gfx 안에. 컨텍스트 손실 시 gfx가 "만드는 방법"(경로/정점 데이터)을 기억했다가 재생성 → 지금 ShaderLib 방식을 Mesh/Texture로 일반화
2. **DrawCmd = mesh + material + model 행렬**: Grid/Arrow2D가 각자 VAO를 관리하는 대신 Mesh를 등록하고 핸들로 제출
3. **엔티티는 단순하게 시작**: `variant`/`optional` 덩어리 대신, 컴포넌트 종류별 `vector` + 엔티티 id. 본격 ECS 라이브러리는 필요해질 때
4. **Lua는 게임 로직, C++은 시스템**: Lua가 GL을 직접 건드리지 않고 엔티티/컴포넌트만 조작

## 3. 단계별 계획

각 단계 끝에 **Android와 데스크톱 양쪽에서 같은 화면**이 나와야 다음으로 넘어감.
0.5단계 이후로는 각 단계 끝에 **기준 씬을 태블릿에서 다시 측정**하고 숫자를 기록 (5번 참고).

| 단계 | 할 일 | 완료 기준 |
|---|---|---|
| **0. 정리** | 작은 버그 수정, core에서 Android 헤더 제거, `App`을 플랫폼 부분/엔진 부분으로 분리, Lua 바인딩·소켓을 공유 코드로 | 데스크톱이 Android와 같은 파이프라인으로 빌드·실행 |
| **0.5. 계측** | 프레임 시간(평균/최악), 드로우 콜 수, 프레임당 할당 횟수를 화면에 텍스트로 표시. 기준 씬과 예산 정하기 | 기준 씬의 숫자가 `mdFiles`에 기록됨 |
| **1. gfx 리소스** | `Mesh`(VAO/VBO/EBO), `Texture`, 핸들 + 컨텍스트 손실 재생성 | 텍스처 입힌 사각형이 백그라운드 갔다 와도 유지 |
| **2. Renderer 강화** | DrawCmd에 mesh/material/model, 인덱스 드로우, uniform location 캐시, 투명 패스 거리 정렬 | Grid·Arrow2D·rect가 전부 Mesh 핸들로 그려짐 |
| **3. Scene** | 엔티티 + Transform + MeshRenderer, Lua에서 생성/이동 API | Lua로 오브젝트 100개 띄워 움직이기 |
| **4. 루프·입력** | 고정 timestep 업데이트, InputState(멀티터치/키보드/마우스), 액션 매핑 | 같은 Lua 코드로 터치와 키보드 둘 다 조작 |
| **5. 2D 게임 기능** | 스프라이트 배치, 아틀라스, AABB 충돌, RigidBody 적분, (오디오는 이때 라이브러리 조사) | — |
| **6. 첫 게임** | Pong/벽돌깨기 수준을 Lua만으로 | APK와 데스크톱 둘 다 플레이 가능 |
| **7. 이후** | 3D 메시 로드, 조명, UI, 씬 저장/로드 | 6번에서 부족했던 것 위주로 선택 |


## 4. 0단계 세부 순서

1. 작은 버그 4개 수정 (Rigidbody 반환형, pragma once, rect null 체크, xmake freetype/주석)
2. `core/input.h`에서 `<android/input.h>` 제거 (이미 쓰지 않음)
3. `App`을 둘로: 플랫폼 상태(EGL dpy/surf/ctx, GLFW window) / 엔진 상태(shaders, cam, renderer, lua …)
4. `luaScript.cpp`에서 Android include 제거, `a.ctx != EGL_NO_CONTEXT` 같은 검사는 엔진의 `hasContext` 플래그로
5. 소켓 핫리로드를 공유 파일 하나로 (데스크톱 쪽에 Windows 분기가 이미 있음)
6. 데스크톱 main을 Android main과 같은 흐름으로 다시 작성
7. 폴더를 2번 구조로 이동 (동작이 같아진 뒤에)

## 5. 성능 방향: "빠른 엔진"의 정의

**"빠름"의 종류와 승산**

| 종류 | 의미 | 승산 |
|---|---|---|
| 반복 속도 | 코드 고침 → 화면 반영까지 시간 | 높음. 핫 리로드가 있고 태블릿 한 대에서 개발·실행 |
| 가벼움 | APK 크기, 실행 → 첫 프레임 시간 | 높음. Java 없는 NativeActivity |
| 런타임 성능 | 저사양 모바일 GPU에서 프레임 시간 | 중간. 2D와 모바일에 집중하면 가능 |
| 범용 3D 성능 | 대형 씬, 조명, 그림자 | 낮음 |

**제안 포지션**: 저사양 안드로이드에서 즉시 켜지고, 기기 위에서 바로 고치는 초경량 2D 엔진.

**측정 과정**
1. 계측부터: 프레임 시간(평균/최악), 드로우 콜 수, 프레임당 할당 횟수를 화면에 표시 (기존 텍스트 렌더러 사용)
2. 기준 씬 고정: 예) 사각형 1만 개 움직이기. 같은 씬, 같은 기기
3. 예산 정하기: 예) 프레임 16.6ms 이하, APK 2MB 이하, 첫 프레임 300ms 이하 (숫자는 예시, 미확정)
4. 기능 추가마다 재측정, 숫자를 `mdFiles`에 기록. 나빠지면 그 커밋에서 원인 찾기
5. 측정은 반드시 태블릿에서. 모바일 GPU는 타일 기반이라 병목이 데스크톱과 다름 (오버드로우, 프레임 중간의 렌더 타깃 전환)

**지금 코드에서 느려질 지점** (오브젝트가 많아지면 문제 되는 순서)
- `DrawCmd`마다 `std::vector<ShaderUniform>` 할당, 유니폼을 문자열 이름으로 전달
- 사각형 하나 = 드로우 콜 하나. 배칭/인스턴싱 없음 (GLES 3.0도 인스턴싱 지원)
- Lua가 오브젝트마다 C++ 함수를 호출 → 경계를 넘는 횟수가 오브젝트 수에 비례
- 매 프레임 `stable_sort` → 커맨드가 수천 개가 되면 정렬 키를 정수 하나로

**설계 원칙**
- 데이터를 배열로: 컴포넌트 종류별 연속 배열 (2번의 설계 결정 3과 같은 방향)
- 프레임 중 할당 0: 시작할 때 버퍼를 잡아 두고 재사용
- Lua는 지시, C++은 반복: Lua는 데이터를 바꾸고, 반복문은 C++ 시스템이 돎
