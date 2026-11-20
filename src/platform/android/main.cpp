//
// Android: NativeActivity + EGL + GLES3 + Lua(sol2) 핫 리로드 테스트
//
// 스크립트 전송: Termux에서 ./push.sh  (127.0.0.1:7777, NUL 바이트로 끝)
#include "gl.h"
#include <EGL/egl.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android_native_app_glue.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <time.h>

#include "core/App.h"
#include "core/appFrame.h"
#include "core/defaultScript.h"
#include "core/file.h"
#include "core/hotReload.h"
#include "core/log.h"
#include "core/luaScript.h"
#include "core/shader.h"

static const int PORT = 7777;
// APK 안의 assets/scripts/main.lua (build_apk.sh 가 src/core/main.lua 를 복사)
static const char *SCRIPT_ASSET = "scripts/main.lua";


static const char *VsDir = "shaders/VertexShader.glsl";
static const char *FsDir = "shaders/ButterflyFgSh_v2.glsl";

// ---------------- INPUT ----------------
// 안드로이드 키 코드 → char (데스크톱과 같은 규칙: 문자는 대문자)
static char keyChar(int32_t code) {
    if (code >= AKEYCODE_A && code <= AKEYCODE_Z)
        return (char)('A' + (code - AKEYCODE_A));
    if (code >= AKEYCODE_0 && code <= AKEYCODE_9)
        return (char)('0' + (code - AKEYCODE_0));
    switch (code) {
    case AKEYCODE_SPACE:
        return ' ';
    case AKEYCODE_ENTER:
        return '\n';
    case AKEYCODE_TAB:
        return '\t';
    case AKEYCODE_DEL:
        return '\b';
    default:
        return 0;
    }
}

static int32_t onInput(android_app *app, AInputEvent *event) {
    App *a = (App *)app->userData;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
        char c = keyChar(AKeyEvent_getKeyCode(event));
        if (!c)
            return 0; // BACK 등은 시스템에 넘김
        int32_t act = AKeyEvent_getAction(event);
        if (act == AKEY_EVENT_ACTION_DOWN)
            a->inputEvents.push_back({InputType::KeyDown, 0, 0, 0, c});
        else if (act == AKEY_EVENT_ACTION_UP)
            a->inputEvents.push_back({InputType::KeyUp, 0, 0, 0, c});
        return 1;
    }

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return 0;

    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);
    switch (action) {
    case AMOTION_EVENT_ACTION_DOWN:
        a->inputEvents.push_back({InputType::PointerDown, 0, x, y, 0});
        break;
    case AMOTION_EVENT_ACTION_UP:
        a->inputEvents.push_back({InputType::PointerUp, 0, x, y, 0});
        break;
    case AMOTION_EVENT_ACTION_MOVE:
        a->inputEvents.push_back({InputType::PointerMove, 0, x, y, 0});
        break;
    }
    return 1;
}

// ---------------- GL ----------------
static bool initGL(App &a, ANativeWindow *win) {
    a.dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(a.dpy, nullptr, nullptr);
    const EGLint cfgAttr[] = {EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES3_BIT,
                              EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT,
                              EGL_RED_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              24,
                              EGL_NONE};
    EGLConfig cfg;
    EGLint n = 0;
    if (!eglChooseConfig(a.dpy, cfgAttr, &cfg, 1, &n) || n < 1)
        return false;
    a.surf = eglCreateWindowSurface(a.dpy, cfg, win, nullptr);
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    a.ctx = eglCreateContext(a.dpy, cfg, EGL_NO_CONTEXT, ctxAttr);
    if (!eglMakeCurrent(a.dpy, a.surf, a.surf, a.ctx))
        return false;

    a.glCurrent = true;
    appInitGL(a);

    LOGI("GL init ok: %s / %s", (const char *)glGetString(GL_VERSION),
         (const char *)glGetString(GL_RENDERER));
    return true;
}

static void termGL(App &a) {
    if (a.dpy == EGL_NO_DISPLAY)
        return;
    if (a.glCurrent) // 컨텍스트가 살아 있을 때 GL 리소스 해제
        appTermGL(a);
    a.glCurrent = false;
    eglMakeCurrent(a.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (a.ctx != EGL_NO_CONTEXT)
        eglDestroyContext(a.dpy, a.ctx);
    if (a.surf != EGL_NO_SURFACE)
        eglDestroySurface(a.dpy, a.surf);
    eglTerminate(a.dpy);
    a.dpy = EGL_NO_DISPLAY;
    a.surf = EGL_NO_SURFACE;
    a.ctx = EGL_NO_CONTEXT;
}

static double now() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ---------------- 프레임 ----------------
static void frame(App &a, float dt) {
    EGLint w = 0, h = 0;
    eglQuerySurface(a.dpy, a.surf, EGL_WIDTH, &w);
    eglQuerySurface(a.dpy, a.surf, EGL_HEIGHT, &h);
    appFrame(a, dt, (int)w, (int)h);
    eglSwapBuffers(a.dpy, a.surf);
}

static void onCmd(android_app *app, int32_t cmd) {
    App &a = *(App *)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window && !initGL(a, app->window))
            termGL(a);
        break;
    case APP_CMD_TERM_WINDOW:
        termGL(a);
        break;
    }
}

void android_main(android_app *app) {
    Shader::assets = app->activity->assetManager;
    setAssetManager(app->activity->assetManager);

    App a;
    app->userData = &a;
    app->onAppCmd = onCmd;
    app->onInputEvent = onInput;

    a.shaders.add("main", VsDir, FsDir);
    a.grid.shaderRegister(a.shaders);
    a.grid.setPlane(1); // XY 격자
    a.arrow2D.shaderRegister(a.shaders);
    a.balls.shaderRegister(a.shaders);
    a.planeBalls.shaderRegister(a.shaders);
    a.modeBtn.shaderRegister(a.shaders);
    a.specters.shaderRegister(a.shaders);
    a.post.shaderRegister(a.shaders);
    a.font.load("fonts/pixel.ttf", 32.0f);

    loadLuaScript(a, DEFAULT_SCRIPT);
    // 데스크톱처럼 main.lua 로 시작 (없으면 DEFAULT_SCRIPT 유지)
    auto buf = readFile(SCRIPT_ASSET);
    if (!buf.empty()) {
        std::string r = loadLuaScript(a, std::string(buf.begin(), buf.end()));
        LOGI("%s: %s", SCRIPT_ASSET, r.c_str());
    }
    hotReloadOpen(a, PORT);

    double prev = now();
    while (true) {
        int events;
        android_poll_source *src;
        int timeout = a.glReady() ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events, (void **)&src) >=
               0) {
            if (src)
                src->process(app, src);
            if (app->destroyRequested) {
                termGL(a);
                hotReloadClose(a);
                return;
            }
            timeout = a.glReady() ? 0 : -1;
        }

        double t = now();
        float dt = (float)(t - prev);
        prev = t;
        if (dt > 0.1f)
            dt = 0.1f; // 복귀 직후 튐 방지

        hotReloadPoll(a);
        if (a.glReady())
            frame(a, dt);
    }
}
