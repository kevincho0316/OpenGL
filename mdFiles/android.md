# Android 빌드 & 실행 가이드 (Termux)

## 한 줄 요약

```bash
cd ~/projects/OpenGL && xmake && ./build_apk.sh
cp build/apk/gltest.apk ~/storage/downloads/
termux-open --chooser ~/storage/downloads/gltest.apk   # → "패키지 설치 프로그램" 선택
am start -n com.kevin.gltest/android.app.NativeActivity
```

> ⚠️ 모든 명령은 **프로젝트 루트(`~/projects/OpenGL`)** 에서 실행. 다른 폴더에서 하면 `build/...` 경로가 없어 조용히 실패함.

---

## 1. 최초 1회 준비

```bash
pkg install zip openjdk-17
termux-setup-storage          # ~/storage/downloads 생성 (저장소 권한 허용)
```

xmake 안드로이드 설정 (플랫폼 바꿀 때마다 다시):

```bash
xmake f -p android -a arm64-v8a --ndk=$ANDROID_NDK --android_sdk=$ANDROID_SDK_ROOT --ndk_sdkver=24 -c -y
```

- `--sdk` 아님, **`--android_sdk`**
- 데스크톱으로 돌아갈 때: `xmake f -p linux -c -y`

---

## 2. 빌드 → APK

```bash
xmake            # .so 생성
./build_apk.sh   # 마지막에 "완료: build/apk/gltest.apk" 나오면 성공
```

`.so` 확인: `find build -name "*.so"`

---

## 3. 설치

```bash
cp build/apk/gltest.apk ~/storage/downloads/
termux-open --chooser ~/storage/downloads/gltest.apk
```

- 선택 창에서 **패키지 설치 프로그램** 선택 (Termux 선택 ❌)
- "Save file in ~/downloads/" 창이 뜨면 → Termux로 열린 것. **취소** 후 `--chooser`로 다시
- 대안: 내 파일 → **내장 저장공간 → Download → gltest.apk** (카테고리 화면은 갱신이 늦어 안 보일 수 있음)
- "출처를 알 수 없는 앱" 경고 → 해당 앱(Termux/내 파일) 설치 허용

## 4. 실행

앱 목록에서 **GLTest**, 또는:

```bash
am start -n com.kevin.gltest/android.app.NativeActivity
```

정상: 청록 배경 + 주황 사각형

---

## 프로젝트 구성 요소

| 파일 | 역할 |
|---|---|
| `src/platform/android/main.cpp` | `android_main` + EGL + GLES3 진입점 |
| `src/gl.h` | `USE_GLES`면 `<GLES3/gl3.h>`, 아니면 `<glad/gl.h>` |
| `android/AndroidManifest.xml` | NativeActivity, `hasCode="false"`, `lib_name="main"` |
| `build_apk.sh` | aapt2 → zip → zipalign → apksigner |
| `~/.android/debug.keystore` | 서명 키 (스크립트가 처음에 자동 생성, 비번 `android`) |

### xmake.lua 안드로이드 타깃 필수 항목

```lua
add_files(path.join(os.getenv("ANDROID_NDK"), "sources/android/native_app_glue/android_native_app_glue.c"))
add_includedirs(path.join(os.getenv("ANDROID_NDK"), "sources/android/native_app_glue"))
add_syslinks("android", "EGL", "GLESv3", "log")
add_ldflags("-u ANativeActivity_onCreate", {force = true})  -- 없으면 실행 즉시 종료
add_defines("USE_GLES")
```

`.so` 이름이 `libmain.so`가 아니어도 `build_apk.sh`가 APK 안에서 `libmain.so`로 바꿔 넣음.

---

## 겪었던 문제 & 해결

| 증상 | 원인 | 해결 |
|---|---|---|
| `unknown type name 'mkdir'` | `cat > ... <<'EOF'` 블록을 vim에 붙여 넣음 | 이런 블록은 **터미널에** 붙여 넣기 |
| `'glad/gl.h' file not found` | core 코드가 glad를 직접 include | `#include "../gl.h"`로 교체 + `USE_GLES` 정의 |
| `termux-open` 아무 반응 없음 | 잘못된 폴더에서 실행 (파일 없음, 에러도 안 뜸) | 프로젝트 루트에서 실행 / 절대경로 사용 |
| "Save file in ~/downloads/" 창 | APK가 Termux로 열림 | `termux-open --chooser` → 패키지 설치 프로그램 |
| 내 파일에서 APK 안 보임 | 카테고리 화면 미갱신 | 내장 저장공간 → Download 폴더 직접 열기 |

### 코드 작성 시 주의
- GLES에 없는 함수 쓰지 말 것: `glPolygonMode` 등 → `#ifndef USE_GLES`로 분리
- 셰이더는 `#version 300 es` + `precision mediump float;`
- 창 크기: 콜백 대신 매 프레임 `eglQuerySurface`
- `APP_CMD_TERM_WINDOW`에서 GL 리소스 전부 해제, `APP_CMD_INIT_WINDOW`에서 재생성

