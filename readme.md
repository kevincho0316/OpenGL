# OpenGL

하나의 C++ 코드베이스로 **Android / Linux / Windows**에서 도는 작은 OpenGL 렌더러.
렌더 루프는 Lua 스크립트로 조종하고, 앱을 끄지 않고 스크립트를 바꿔 끼울 수 있다(핫 리로드).

Android Studio·Gradle·CMake 없이 **xmake**만 쓰며, APK도 Termux 안에서 직접 만든다.

| | Android | 데스크톱 (Linux / Windows) |
|---|---|---|
| 진입점 | NativeActivity (`android_main`, Java 코드 없음) | GLFW |
| GL | EGL + OpenGL ES 3.0 | glad + OpenGL 3.3 core |
| 산출물 | `libmain.so` → `gltest.apk` | 실행 파일 `app` |

## 기능

- **Renderer**: `DrawCmd`를 `submit` → `flush`. 패스(Opaque / Transparent / Overlay / UI) 순으로 정렬해 그림
- **ShaderLib**: 셰이더를 이름으로 등록. 경로를 기억해 두었다가 GL 컨텍스트가 사라지면 전부 다시 생성
- **셰이더 공용화**: `#version` 줄은 C++ 로더가 플랫폼에 맞게 붙임. 셰이더 파일 안에서 `#include` 사용 가능
- **Lua(sol2) 스크립팅**: `init()` / `update(dt)`, 유니폼·카메라·텍스트·사각형 API
- **핫 리로드**: TCP `127.0.0.1:7777`로 스크립트 전송. 로드에 실패하면 이전 스크립트 유지
- **카메라**(원근 / 직교), **그리드**, **드래그 화살표**, **텍스트**(stb_truetype 아틀라스)
- **입력**: 터치와 마우스를 같은 `inputEvent`(프레임버퍼 픽셀, 왼쪽 위 원점)로 통일

## 폴더 구조

```
src/
├── gl.h                 USE_GLES면 <GLES3/gl3.h>, 아니면 glad
├── core/                공유 코드 (GL 호출은 여기)
│   ├── App.h            앱 상태 전체
│   ├── appFrame.*       appInitGL / appTermGL / appFrame — 플랫폼 공통 프레임
│   ├── render.*         Renderer, DrawCmd, Pass
│   ├── shader.* shaderLib.* uniform.h
│   ├── luaScript.*      Lua 바인딩
│   ├── hotReload.*      핫 리로드 소켓 서버
│   ├── camera.* Grid.* arrow2D.* font.* textRenderer.*
│   ├── file.*           readFile() — 데스크톱 fopen / Android AAssetManager
│   ├── input.h log.h
│   └── main.lua         기본 스크립트
├── platform/
│   ├── android/main.cpp   EGL 컨텍스트, 입력, 메인 루프
│   └── desktop/main.cpp   GLFW 창, 입력, 메인 루프, 파일 감시
└── types/functionProto.lua   Lua API 타입 정의 (에디터 자동완성용)
assets/        shaders/, fonts/ — APK에 그대로 들어감
android/       AndroidManifest.xml
third_party/   sol2 (서브모듈), stb_truetype.h
mdFiles/       학습 노트, Android 빌드 가이드
```

플랫폼 `main.cpp`는 창·컨텍스트 생성, 이벤트 수집, 버퍼 스왑만 하고 나머지는 `appFrame()`에 맡긴다.

## 받기

```bash
git clone https://github.com/kevincho0316/OpenGL
cd OpenGL
git submodule update --init third_party/sol2
```

## 데스크톱 빌드

glfw, glad, glm, lua는 xmake가 받아 온다.

```bash
xmake f -p linux -c -y      # Windows는 -p windows
xmake
xmake run
```

- 실행 위치는 프로젝트 루트로 고정되어 있다 (`assets/`를 상대 경로로 읽음)
- `src/core/main.lua`를 저장하면 자동으로 리로드
- `F5` 셰이더 리로드, `Esc` 종료

## Android 빌드 (Termux)

준비물

- NDK / SDK: [lzhiyong/termux-ndk](https://github.com/lzhiyong/termux-ndk)의 aarch64 빌드
- 환경 변수 `ANDROID_NDK`, `ANDROID_SDK_ROOT`
- `pkg install zip openjdk-17 glm`

```bash
xmake f -p android -a arm64-v8a --ndk=$ANDROID_NDK --android_sdk=$ANDROID_SDK_ROOT --ndk_sdkver=24 -c -y
xmake
./build_apk.sh
```

> `--sdk`가 아니라 `--android_sdk`다. `--sdk`는 cross 툴체인으로 잡혀 에러가 난다.

`build_apk.sh`는 aapt2 → zip → zipalign → apksigner 순으로 `build/apk/gltest.apk`를 만든다.
서명 키(`~/.android/debug.keystore`)는 처음 실행할 때 자동으로 생성된다.

설치와 실행

```bash
cp build/apk/gltest.apk ~/storage/downloads/
termux-open --chooser ~/storage/downloads/gltest.apk
am start -n com.kevin.gltest/android.app.NativeActivity
```

설치 문제 해결과 logcat 사용법은 [mdFiles/android.md](mdFiles/android.md) 참고.

## Lua 핫 리로드

앱이 켜져 있는 상태에서:

```bash
./push.sh              # src/core/main.lua 전송 → 리로드
./push.sh 파일.lua     # 지정한 파일 전송
./push.sh status       # 마지막 에러 + print() 출력 조회
```

- 로드 에러: 이전 스크립트가 그대로 돈다
- `init` / `update` 실행 중 에러: 배경이 빨간색으로 바뀐다 → `./push.sh status`로 확인
- Android에서는 소켓 때문에 `INTERNET` 권한을 쓴다 (루프백 전용)

## Lua API

스크립트는 `init()`(로드 후 1회)과 `update(dt)`(매 프레임)를 정의한다.
쓸 수 있는 표준 라이브러리는 base, math, string, table.

| 분류 | 함수 |
|---|---|
| 타입 | `vec3.new(x,y,z)`, `vec4.new(x,y,z,w)` |
| 화면 | `clear_color(r,g,b)` |
| 유니폼 | `setBool` / `setInt` / `setFloat` / `setVec3` / `setVec4` `(shader, name, value)` |
| 변환 | `setTrans(angle, axis, scale)` — `main` 셰이더의 `transform` |
| 카메라 | `camPos(v)`, `camRot(yaw,pitch)`, `camLookAt(v)`, `camMove(v)`, `camPerspective([fov=45])`, `camOrtho([size=1])`, `camClip(near,far)`, `camGetPos()`, `camFront()` |
| 그리기 | `rect(shader, vec4 rect, vec3 color)` |
| 텍스트 | `setFont(path, size)`, `setText(text, x, y, size, vec4 color)` |
| 디버그 | `print(...)` — `./push.sh status`로 읽음 |

```lua
local t = 0
function update(dt)
    t = t + dt
    clear_color(0.1, 0.1, 0.15)
    camPerspective(60)
    camPos(vec3.new(math.sin(t) * 5, 2, math.cos(t) * 5))
    camLookAt(vec3.new(0, 0, 0))
    setText("t: " .. string.format("%.1f", t), 50, 50, 5, vec4.new(1, 1, 1, 1))
end
```

전체 시그니처는 [src/types/functionProto.lua](src/types/functionProto.lua)에 있다.

## 셰이더 작성 규칙

- 파일에 `#version`을 쓰지 않는다. 로더가 GLES에서는 `#version 300 es` + `precision mediump float;`, 데스크톱에서는 `#version 330 core`를 붙인다
- 매 프레임 자동으로 들어오는 유니폼: `view`, `projection`, `uTime`
- 공유 코드에서는 GLES 3.0에 없는 기능을 쓰지 않는다 (`glPolygonMode`, 지오메트리 셰이더 등). 필요하면 `#ifndef USE_GLES`로 분리

## 코드 작성 규칙

- GL 호출은 렌더 스레드에서만, GL 헤더는 `gl.h`를 통해서만 include
- Android는 창이 사라지면(`APP_CMD_TERM_WINDOW`) GL 리소스를 전부 해제하고 `APP_CMD_INIT_WINDOW`에서 다시 만든다. GL 리소스를 추가하면 `appInitGL` / `appTermGL` 양쪽에 넣을 것
- 에셋은 `readFile()`로만 읽는다. 경로는 `assets/` 기준 (`"shaders/Grid.vert"`)
- 출력은 `printf` 대신 `LOGI` / `LOGW` / `LOGE` (`core/log.h`)

## 앞으로

Mesh / Texture 핸들과 컨텍스트 손실 복구 → 인덱스 드로우와 머티리얼 → 엔티티 / 씬 → 멀티터치·키 입력 → Lua만으로 만드는 첫 2D 게임.

## 사용한 라이브러리

[xmake](https://xmake.io) · [GLFW](https://www.glfw.org) · [glad](https://github.com/Dav1dde/glad) · [glm](https://github.com/g-truc/glm) · [Lua 5.4](https://www.lua.org) · [sol2](https://github.com/ThePhD/sol2) · [stb_truetype](https://github.com/nothings/stb)
