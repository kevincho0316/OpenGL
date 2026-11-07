//
// Android: NativeActivity + EGL + GLES3 + Lua(sol2) 핫 리로드 테스트
//
// 스크립트 전송: Termux에서 ./push.sh  (127.0.0.1:7777, NUL 바이트로 끝)
#include "../../gl.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include <EGL/egl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <android/asset_manager.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#define SOL_ALL_SAFETIES_ON 1
#include "core/camera.h"
#include "shader.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
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

function update(dt)
  t = t + dt
  clear_color(0.0, 0.0, 0.0)

  setFloat("uTime", t)
  setFloat("uToffset", 0.06)
  setFloat("uboost", 2.0)
  setFloat("ucontrast", 0.8)
  setFloat("uConCenter", 0.8)
  setFloat("uradius", 0.5)
  setFloat("ufadeRad", 0.5)
  setFloat("NOISE_AMT", 0.6)
  setTrans( 180.0, vec3.new(0.0, 0.0, 1.0),
                   vec3.new(0.5, -0.5, 0.0));


  local a = t * 2 * math.pi / 5   -- 5초에 한 바퀴
  rect(vec4.new(0, 0, 6, 13),
       vec3.new(0.5 + math.sin(a), 0.6, 0.85))
end
)LUA";

static const char *VsDir = "shaders/VertexShader.glsl";
static const char *FsDir = "shaders/ButterflyFgSh.glsl";

struct App {
  // EGL / GL (창이 없어지면 전부 해제 → 다시 생성)
  EGLDisplay dpy = EGL_NO_DISPLAY;
  EGLSurface surf = EGL_NO_SURFACE;
  EGLContext ctx = EGL_NO_CONTEXT;
  GLuint vao = 0, vbo = 0;
  float clear[3] = {0, 0, 0};
  Shader shader;
  Camera cam; // 스크립트 리로드해도 유지됨
  // Lua (GL 객체를 갖지 않으므로 컨텍스트 손실과 무관하게 유지)
  std::unique_ptr<sol::state> lua;
  std::string lastError; // 런타임 에러 (status 로 조회)
  std::string printBuf;  // print() 출력 (status 로 조회)
  int listenFd = -1;
};

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

  a.shader = Shader(VsDir, FsDir);

  const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
  glGenVertexArrays(1, &a.vao);
  glGenBuffers(1, &a.vbo);
  glBindVertexArray(a.vao);
  glBindBuffer(GL_ARRAY_BUFFER, a.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  LOG("GL init ok");
  return true;
}

static void termGL(App &a) {
  if (a.dpy == EGL_NO_DISPLAY)
    return;
  if (a.ctx != EGL_NO_CONTEXT) {
    glDeleteProgram(a.shader.ID);
    glDeleteBuffers(1, &a.vbo);
    glDeleteVertexArrays(1, &a.vao);
  }
  a.shader.ID = a.vao = a.vbo = 0;
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

void setTrans(App &a, float angle, glm::vec3 angleVec, glm::vec3 scale) {
  glm::mat4 trans = glm::mat4(1.0f);
  trans = glm::rotate(trans, glm::radians(angle), angleVec);
  trans = glm::scale(trans, scale);
  unsigned int transformLoc = glGetUniformLocation(a.shader.ID, "transform");
  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));
}

// ---------------- Lua ----------------
// 성공 시 "ok", 실패 시 에러 메시지. 실패하면 이전 스크립트 유지.
static std::string loadScript(App &a, const std::string &src) {
  auto L = std::make_unique<sol::state>();
  L->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                    sol::lib::table);

  L->new_usertype<glm::vec3>(
      "vec3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
      "x", &glm::vec3::x, "y", &glm::vec3::y, "z", &glm::vec3::z);

  L->new_usertype<glm::vec4>(
      "vec4",
      sol::constructors<glm::vec4(), glm::vec4(float, float, float, float)>(),
      "x", &glm::vec4::x, "y", &glm::vec4::y, "z", &glm::vec4::z, "w",
      &glm::vec4::w);

  L->set_function("clear_color", [&a](float r, float g, float b) {
    a.clear[0] = r;
    a.clear[1] = g;
    a.clear[2] = b;
  });

  L->set_function("setBool", [&a](const std::string &n, bool v) {
    a.shader.setBool(n, v);
  });
  L->set_function("setInt",
                  [&a](const std::string &n, int v) { a.shader.setInt(n, v); });

  L->set_function("setFloat", [&a](const std::string &n, float v) {
    a.shader.setFloat(n, v);
  });

  L->set_function("setVec3", [&a](const std::string &n, glm::vec3 v) {
    a.shader.setVec3(n, v);
  });
  L->set_function("setVec4", [&a](const std::string &n, glm::vec4 v) {
    a.shader.setVec4(n, v);
  });

  L->set_function("setTrans",
                  [&a](float angle, glm::vec3 axis, glm::vec3 scale) {
                    setTrans(a, angle, axis, scale);
                  });

  // ---- camera ----
  L->set_function("camPos", [&a](glm::vec3 p) { a.cam.position = p; });
  L->set_function("camRot", [&a](float yaw, float pitch) {
    a.cam.setRotation(yaw, pitch);
  });
  L->set_function("camLookAt", [&a](glm::vec3 t) { a.cam.lookAt(t); });
  L->set_function("camMove", [&a](glm::vec3 d) { a.cam.move(d); });
  L->set_function("camPerspective", [&a](sol::optional<float> fov) {
    a.cam.perspective = true;
    a.cam.fov = fov.value_or(45.0f);
  });
  L->set_function("camOrtho", [&a](sol::optional<float> size) {
    a.cam.perspective = false;
    a.cam.orthoSize = size.value_or(1.0f);
  });
  L->set_function("camClip", [&a](float n, float f) {
    a.cam.zNear = n;
    a.cam.zFar = f;
  });
  L->set_function("camGetPos", [&a]() { return a.cam.position; });
  L->set_function("camFront", [&a]() { return a.cam.front(); });

  L->set_function("rect", [&a](glm::vec4 rect, glm::vec3 color) {
    if (!a.shader.ID)
      return;

    a.shader.setVec4("uRect", rect);
    a.shader.setVec3("uColor", color);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  });

  L->set_function("print", [&a](sol::this_state ts, sol::variadic_args va) {
    sol::state_view lv(ts);
    sol::protected_function tostr = lv["tostring"];
    std::string line;
    for (auto v : va) {
      std::string s = tostr(v);
      line += s + "\t";
    }
    LOG("lua: %s", line.c_str());
    a.printBuf += line + "\n";
    if (a.printBuf.size() > 4096)
      a.printBuf.erase(0, a.printBuf.size() - 4096);
  });

  auto r = L->safe_script(src, sol::script_pass_on_error);
  if (!r.valid()) {
    sol::error e = r;
    LOG("load error: %s", e.what());
    return std::string("load error: ") + e.what();
  }
  a.lua = std::move(L);
  a.lastError.clear();
  LOG("script loaded (%zu bytes)", src.size());
  return "ok";
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
    reply =
        "error: " + (a.lastError.empty() ? std::string("none") : a.lastError) +
        "\n--- print ---\n" + a.printBuf;
    a.printBuf.clear();
  } else {
    reply = loadScript(a, src);
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
  a.shader.use();
  glBindVertexArray(a.vao);
  float aspect = h > 0 ? (float)w / (float)h : 1.0f;
  a.shader.setMat4("view", a.cam.view());
  a.shader.setMat4("projection", a.cam.projection(aspect));

  if (a.lua && a.lastError.empty()) {
    sol::protected_function upd = (*a.lua)["update"];
    if (upd.valid()) {
      auto r = upd(dt);

      if (!r.valid()) {
        sol::error e = r;
        a.lastError = e.what();
        LOG("update error: %s", e.what());
        a.clear[0] = 0.5f;
        a.clear[1] = 0.0f;
        a.clear[2] = 0.0f; // 빨간 배경 = 에러
      }
    }
  }
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
  App a;
  app->userData = &a;
  app->onAppCmd = onCmd;
  loadScript(a, DEFAULT_SCRIPT);
  a.listenFd = openServer();

  double prev = now();
  while (true) {
    int events;
    android_poll_source *src;
    int timeout = (a.ctx != EGL_NO_CONTEXT) ? 0 : -1;
    while (ALooper_pollOnce(timeout, nullptr, &events, (void **)&src) >= 0) {
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
    if (a.ctx != EGL_NO_CONTEXT)
      frame(a, dt);
  }
}
