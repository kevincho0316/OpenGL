//
// Android: NativeActivity + EGL + GLES3 + Lua(sol2) 핫 리로드 테스트
//
// 스크립트 전송: Termux에서 ./push.sh  (127.0.0.1:7777, NUL 바이트로 끝)
#include "../../gl.h"
#include "core/arrow2D.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include <EGL/egl.h>

#include <GLES3/gl3.h>
#include <android/input.h>
#include <cstdint>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "log.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#define SOL_ALL_SAFETIES_ON 1

#include "core/App.h"
#include "core/file.h"
#include "core/luaScript.h"
#include "core/shader.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sol/sol.hpp>
#include <string>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "gltest", __VA_ARGS__)

static const int PORT = 7777;

static const char *DEFAULT_SCRIPT = R"LUA(

local t = 0
function init()
    --setFont('/fonts/pixel.ttf', 35.0)
end

function update(dt)
    clear_color(0.0, 0.0, 0.0)

    setFloat(("main","uToffset", 0.06)
    setFloat(("main","uboost", 2.0)
    setFloat(("main","ucontrast", 0.8)
    setFloat(("main","uConCenter", 0.8)
    setFloat(("main","uradius", 0.5)
    setFloat(("main","ufadeRad", 0.5)
    setFloat(("main","NOISE_AMT", 0.6)
    setTrans("main",(t * 20, vec3.new(0.0, 0.0, 1.0),
        vec3.new(0.5, -0.5, 0.0));



    -- 카메라 테스트: 원점 주위를 공전하면서 위아래로 흔들림
    camPerspective(60)
    local orbit = t * 0.8
    local radius = 7
    local cp = vec3.new(math.sin(orbit) * radius * math.sin(t), math.sin(t * 0.5) * 3, math.cos(orbit) * radius)

    camPos(cp)
    camLookAt(vec3.new(0, 0, 0))

    --setText("x: " .. string.format("%.2f", cp.x), 250.0, 250.0, 5, vec4.new(1, 1, 1, 1))
    --setText("y: " .. string.format("%.2f", cp.y), 250.0, 400.0, 5, vec4.new(1, 1, 1, 1))
    --setText("z: " .. string.format("%.2f", cp.z), 250.0, 550.0, 5, vec4.new(1, 1, 1, 1))

    local a = t * 2 * math.pi / 5 -- 5초에 한 바퀴
    rect(vec4.new(0, 0, 6, 13),
        vec3.new(0.5 + math.sin(a), 0.6, 0.85))
end
)LUA";

static const char *VsDir = "shaders/VertexShader.glsl";
static const char *FsDir = "shaders/ButterflyFgSh.glsl";

// ---------------- INPUT ----------------
static int32_t onInput(android_app *app, AInputEvent *event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }
    int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    float x = AMotionEvent_getX(event, 0);
    float y = AMotionEvent_getY(event, 0);
    App *a = (App *)app->userData;
    // Debug purpose
    char buf[64];
    switch (action) {
    case AMOTION_EVENT_ACTION_DOWN: {
        a->inputEvents.push_back({InputType::PointerDown, 0, x, y, 0});
        snprintf(buf, sizeof(buf), "INPUT::Action::DOWN::(%.0f,%.0f)", x, y);
        LOGI("%s", buf);
        break;
    }
    case AMOTION_EVENT_ACTION_UP: {
        a->inputEvents.push_back({InputType::PointerUp, 0, x, y, 0});
        snprintf(buf, sizeof(buf), "INPUT::Action::UP::(%.0f,%.0f)", x, y);
        LOGI("%s", buf);
        break;
    }
    case AMOTION_EVENT_ACTION_MOVE: {
        a->inputEvents.push_back({InputType::PointerMove, 0, x, y, 0});
        snprintf(buf, sizeof(buf), "INPUT::Action::MOVE::(%.0f,%.0f)", x, y);
        LOGI("%s", buf);
        break;
    }
    }
    /////////////////////
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

    a.shaders.add("main", VsDir, FsDir);

    const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    glGenVertexArrays(1, &a.vao);
    glGenBuffers(1, &a.vbo);
    glBindVertexArray(a.vao);
    glBindBuffer(GL_ARRAY_BUFFER, a.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    a.grid.init();
    a.grid.setPlane(1); // XY 격자

    a.arrow2D.init();
    a.shaders.recreateAll();
    LOG("GL init ok");
    return true;
}

static void termGL(App &a) {
    if (a.dpy == EGL_NO_DISPLAY)
        return;
    if (a.ctx != EGL_NO_CONTEXT) {
        a.grid.destroy();
        a.shaders.destroyAll();
        glDeleteBuffers(1, &a.vbo);
        glDeleteVertexArrays(1, &a.vao);
    }
    eglMakeCurrent(a.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (a.ctx != EGL_NO_CONTEXT)
        eglDestroyContext(a.dpy, a.ctx);
    if (a.surf != EGL_NO_SURFACE)
        eglDestroySurface(a.dpy, a.surf);
    eglTerminate(a.dpy);
    a.dpy = EGL_NO_DISPLAY;
    a.surf = EGL_NO_SURFACE;
    a.ctx = EGL_NO_CONTEXT;
    a.shaders.destroyAll();
}
// ---------------- 핫 리로드 소켓 ----------------
static int openServer() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG("socket failed (INTERNET 권한?)");
        return -1;
    }
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in ad{};
    ad.sin_family = AF_INET;
    ad.sin_port = htons(PORT);
    ad.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (sockaddr *)&ad, sizeof ad) < 0 || listen(fd, 2) < 0) {
        LOG("bind/listen failed");
        close(fd);
        return -1;
    }
    fcntl(fd, F_SETFL, O_NONBLOCK);
    LOG("listening on 127.0.0.1:%d", PORT);
    return fd;
}

// 매 프레임 호출. 연결이 있으면 NUL 까지 읽고 처리 후 응답.
//   빈 내용 → 상태 조회 (마지막 에러 + print 출력)
//   내용 있음 → 스크립트 리로드
static void pollSocket(App &a) {
    if (a.listenFd < 0)
        return;
    int c = accept(a.listenFd, nullptr, nullptr);
    if (c < 0)
        return;
    timeval tv{2, 0};
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    std::string src;
    char buf[4096];
    bool done = false;
    while (!done) {
        ssize_t n = recv(c, buf, sizeof buf, 0);
        if (n <= 0)
            break;
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\0') {
                done = true;
                break;
            }
            src += buf[i];
        }
    }

    bool empty = src.find_first_not_of(" \t\r\n") == std::string::npos;
    std::string reply;
    if (empty) {
        reply = "error: " +
                (a.lastError.empty() ? std::string("none") : a.lastError) +
                "\n--- print ---\n" + a.printBuf;
        a.printBuf.clear();
    } else {
        reply = loadLuaScript(a, src);
    }
    reply += "\n";
    send(c, reply.data(), reply.size(), 0);
    close(c);
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
    glViewport(0, 0, w, h);
    glClearColor(a.clear[0], a.clear[1], a.clear[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(a.vao);
    float aspect = h > 0 ? (float)w / (float)h : 1.0f;
    a.t += dt;

    a.renderer.frameUniforms = {
        {"view", a.cam.view()},
        {"projection", a.cam.projection(aspect)},
        {"uTime", a.t},
    };

    a.renderer.flush();

    char buf[64];
    for (inputEvent i : a.inputEvents) {
        switch (i.inputType) {
        case InputType::PointerDown:
            a.dragging = true;
            a.dragStart = a.dragCur = glm::vec2(i.x, i.y);
            break;
        case InputType::PointerMove:
            a.dragCur = glm::vec2(i.x, i.y);
            break;
        case InputType::PointerUp:
            a.dragging = false;
            break;
        default:
            break;
        }
        switch (i.inputType) {
        case InputType::PointerDown: {
            snprintf(buf, sizeof(buf), "DOWN::(%.0f,%.0f)", i.x, i.y);
            break;
        }
        case InputType::PointerUp: {
            break;
        }
        case InputType::PointerMove: {
            snprintf(buf, sizeof(buf), "MOVE::(%.0f,%.0f)", i.x, i.y);
            break;
        }
        default: {
            snprintf(buf, sizeof(buf), "INPUT::Action::Key::(%c)", i.key);
            break;
        }
        }
        a.inputEvents.clear();
    }

    if (a.lua && a.lastError.empty() && a.needInit) {
        a.needInit = false;
        sol::protected_function ini = (*a.lua)["init"];
        if (ini.valid()) {
            auto r = ini();
            if (!r.valid()) {
                sol::error e = r;
                a.lastError = e.what();
                LOGE("init error: %s", e.what());
                a.clear[0] = 0.5f;
                a.clear[1] = 0.0f;
                a.clear[2] = 0.3f; // 빨간 배경 = 에러 (update와 동일)
            }
        }
    }

    if (a.lua && a.lastError.empty()) {
        sol::protected_function upd = (*a.lua)["update"];
        if (upd.valid()) {
            auto r = upd(dt);

            if (!r.valid()) {
                sol::error e = r;
                a.lastError = e.what();
                LOGE("update error: %s", e.what());
                a.clear[0] = 0.5f;
                a.clear[1] = 0.0f;
                a.clear[2] = 0.0f; // 빨간 배경 = 에러
            }
        }
    }
    for (const TextCmd &t : a.texts)
        a.textRenderer.draw(t.s.c_str(), t.x, t.y, t.size, t.color, (int)w,
                            (int)h);
    a.texts.clear();

    glm::vec2 d = a.dragCur - a.dragStart;
    float len = glm::length(d);
    if (a.dragging && len > 1.0f) {
        a.arrow2D.draw(a.dragStart, d, len, glm::vec2(w, h));
    }

    a.grid.draw(a.cam, aspect);

    eglSwapBuffers(a.dpy, a.surf);
}

static void onCmd(android_app *app, int32_t cmd) {
    App &a = *(App *)app->userData;
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window && !initGL(a, app->window))
            termGL(a);
        else
            a.textRenderer.init(a.font);
        break;
    case APP_CMD_TERM_WINDOW:
        a.textRenderer.destroy();
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

    loadLuaScript(a, DEFAULT_SCRIPT);
    a.listenFd = openServer();
    a.font.load("fonts/pixel.ttf", 32.0f);

    double prev = now();
    while (true) {
        int events;
        android_poll_source *src;
        int timeout = (a.ctx != EGL_NO_CONTEXT) ? 0 : -1;
        while (ALooper_pollOnce(timeout, nullptr, &events, (void **)&src) >=
               0) {
            if (src)
                src->process(app, src);
            if (app->destroyRequested) {
                termGL(a);
                if (a.listenFd >= 0)
                    close(a.listenFd);
                return;
            }
            timeout = (a.ctx != EGL_NO_CONTEXT) ? 0 : -1;
        }

        double t = now();
        float dt = (float)(t - prev);
        prev = t;
        if (dt > 0.1f)
            dt = 0.1f; // 복귀 직후 튐 방지

        pollSocket(a);
        if (a.ctx != EGL_NO_CONTEXT) {
            frame(a, dt);
        }
    }
}
