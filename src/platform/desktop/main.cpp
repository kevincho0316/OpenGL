// Desktop: GLFW + glad + GL 3.3 core + Lua(sol2) 핫 리로드 (안드로이드와 동일
// 구조) 스크립트 전송: ./push.sh  (127.0.0.1:7777, NUL 바이트로 끝) 데스크톱
// 전용: src/core/main.lua 저장하면 자동 리로드, F5 = 셰이더 리로드
#include "gl.h"
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "core/App.h"
#include "core/log.h"
#include "core/luaScript.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using sock_t = SOCKET;
#define closesock closesocket
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using sock_t = int;
#define closesock close
#endif

#define LOG(...) (fprintf(stderr, "[gltest] " __VA_ARGS__), fputc('\n', stderr))

static const int PORT = 7777;
static const char *SCRIPT_PATH = "src/core/main.lua";

// main.lua 를 못 읽을 때만 사용
static const char *DEFAULT_SCRIPT = R"LUA(
local t = 0
function update(dt)
  t = t + dt
  clear_color(0.1 + 0.1 * math.sin(t), 0.2, 0.3)
end
)LUA";

static const char *VsDir = "shaders/VertexShader.glsl";
static const char *FsDir = "shaders/ButterflyFgSh.glsl";

static sock_t g_listenFd = (sock_t)-1;
static std::filesystem::file_time_type g_scriptTime{};

// ---------------- 오프스크린 FBO ----------------
// WSL(D3D12 드라이버)에서는 기본 프레임버퍼의 깊이 버퍼가 glClear 로 안 지워짐
// (항상 0 → 깊이 테스트 전부 실패). 직접 만든 FBO 에 그리고 화면으로 blit.
static GLuint g_fbo = 0, g_fboColor = 0, g_fboDepth = 0;
static int g_fboW = 0, g_fboH = 0;

static void destroyFbo() {
  if (g_fbo)
    glDeleteFramebuffers(1, &g_fbo);
  if (g_fboColor)
    glDeleteRenderbuffers(1, &g_fboColor);
  if (g_fboDepth)
    glDeleteRenderbuffers(1, &g_fboDepth);
  g_fbo = g_fboColor = g_fboDepth = 0;
  g_fboW = g_fboH = 0;
}

// 창 크기가 바뀌면 다시 만듦
static void ensureFbo(int w, int h) {
  if (g_fbo && w == g_fboW && h == g_fboH)
    return;
  destroyFbo();
  glGenRenderbuffers(1, &g_fboColor);
  glBindRenderbuffer(GL_RENDERBUFFER, g_fboColor);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
  glGenRenderbuffers(1, &g_fboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, g_fboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glGenFramebuffers(1, &g_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, g_fboColor);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, g_fboDepth);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    LOG("FBO incomplete");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  g_fboW = w;
  g_fboH = h;
}

// ---------------- GL ----------------
static bool initGL(App &a) {
  if (!glfwInit())
    return false;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  a.win = glfwCreateWindow(900, 900, "GLTest", nullptr, nullptr);
  if (!a.win) {
    LOG("Failed to create GLFW window");
    return false;
  }
  glfwMakeContextCurrent(a.win);
  glfwSwapInterval(1);
  if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    LOG("Failed to init GLAD");
    return false;
  }

  const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
  glGenVertexArrays(1, &a.vao);
  glGenBuffers(1, &a.vbo);
  glBindVertexArray(a.vao);
  glBindBuffer(GL_ARRAY_BUFFER, a.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  a.shaders.recreateAll();
  a.grid.init(a.shaders);
  a.arrow2D.init(a.shaders);
  a.textRenderer.init(a.font);

  LOG("GL init ok: %s / %s", (const char *)glGetString(GL_VERSION),
      (const char *)glGetString(GL_RENDERER));
  return true;
}

static void termGL(App &a) {
  if (a.win) {
    destroyFbo();
    a.textRenderer.destroy();
    a.grid.destroy();
    a.arrow2D.destroy();
    glDeleteBuffers(1, &a.vbo);
    glDeleteVertexArrays(1, &a.vao);
    a.vao = a.vbo = 0;
    a.shaders.destroyAll();
    glfwDestroyWindow(a.win);
    a.win = nullptr;
  }
  glfwTerminate();
}

// ---------------- 파일 감시 (데스크톱 전용) ----------------
// main.lua 수정 시간이 바뀌면 다시 읽어서 리로드
static void pollScriptFile(App &a, bool force = false) {
  std::error_code ec;
  auto t = std::filesystem::last_write_time(SCRIPT_PATH, ec);
  if (ec || (!force && t == g_scriptTime))
    return;
  std::ifstream f(SCRIPT_PATH, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  std::string src = ss.str();
  if (src.empty())
    return; // 에디터가 저장 도중일 수 있음 → 다음 프레임에 재시도
  g_scriptTime = t;
  std::string r = loadLuaScript(a, src);
  LOG("%s: %s", SCRIPT_PATH, r.c_str());
}

// ---------------- 핫 리로드 소켓 ----------------
static sock_t openServer() {
#ifdef _WIN32
  WSADATA wsa;
  WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
  sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == (sock_t)-1) {
    LOG("socket failed");
    return (sock_t)-1;
  }
  int one = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof one);
  sockaddr_in ad{};
  ad.sin_family = AF_INET;
  ad.sin_port = htons(PORT);
  ad.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(fd, (sockaddr *)&ad, sizeof ad) < 0 || listen(fd, 2) < 0) {
    LOG("bind/listen failed");
    closesock(fd);
    return (sock_t)-1;
  }
#ifdef _WIN32
  u_long nb = 1;
  ioctlsocket(fd, FIONBIO, &nb);
#else
  fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
  LOG("listening on 127.0.0.1:%d", PORT);
  return fd;
}

// 매 프레임 호출. 연결이 있으면 NUL 까지 읽고 처리 후 응답.
//   빈 내용 → 상태 조회 (마지막 에러 + print 출력)
//   내용 있음 → 스크립트 리로드
static void pollSocket(App &a) {
  if (g_listenFd == (sock_t)-1)
    return;
  sock_t c = accept(g_listenFd, nullptr, nullptr);
  if (c == (sock_t)-1)
    return;
#ifdef _WIN32
  u_long blk = 0;
  ioctlsocket(c, FIONBIO, &blk); // accept 소켓은 non-blocking 을 상속함
  DWORD tv = 2000;
#else
  timeval tv{2, 0};
#endif
  setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

  std::string src;
  char buf[4096];
  bool done = false;
  while (!done) {
    int n = (int)recv(c, buf, sizeof buf, 0);
    if (n <= 0)
      break;
    for (int i = 0; i < n; i++) {
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
    reply = loadLuaScript(a, src);
  }
  reply += "\n";
  send(c, reply.data(), (int)reply.size(), 0);
  closesock(c);
}

// ---------------- 프레임 ----------------
static void frame(App &a, float dt) {
  int w = 0, h = 0;
  glfwGetFramebufferSize(a.win, &w, &h);
  if (w <= 0 || h <= 0) { // 최소화 상태
    glfwSwapBuffers(a.win);
    return;
  }
  ensureFbo(w, h);
  glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
  glViewport(0, 0, w, h);
  glClearColor(a.clear[0], a.clear[1], a.clear[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  float aspect = h > 0 ? (float)w / (float)h : 1.0f;
  a.t += dt;

  a.renderer.frameUniforms = {
      {"view", a.cam.view()},
      {"projection", a.cam.projection(aspect)},
      {"uTime", a.t},
  };

  for (const inputEvent &i : a.inputEvents) {
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
  }
  a.inputEvents.clear();

  if (a.lua && a.lastError.empty() && a.needInit) {
    a.needInit = false;
    sol::protected_function ini = (*a.lua)["init"];
    if (ini.valid()) {
      auto r = ini();
      if (!r.valid()) {
        sol::error e = r;
        a.lastError = e.what();
        LOG("init error: %s", e.what());
        a.clear[0] = 0.5f;
        a.clear[1] = 0.0f;
        a.clear[2] = 0.0f; // 빨간 배경 = 에러
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
        LOG("update error: %s", e.what());
        a.clear[0] = 0.5f;
        a.clear[1] = 0.0f;
        a.clear[2] = 0.0f; // 빨간 배경 = 에러
      }
    }
  }

  glm::vec2 d = a.dragCur - a.dragStart;
  float len = glm::length(d);
  if (a.dragging && len > 1.0f)
    a.arrow2D.draw(a.renderer, a.dragStart, d, len, glm::vec2(w, h));

  a.grid.draw(a.renderer, a.cam, aspect);
  a.renderer.flush();

  for (const TextCmd &t : a.texts)
    a.textRenderer.draw(t.s.c_str(), t.x, t.y, t.size, t.color, w, h);
  a.texts.clear();

  glBindFramebuffer(GL_READ_FRAMEBUFFER, g_fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glfwSwapBuffers(a.win);
}

// ---------------- INPUT ----------------
// 안드로이드 터치와 같은 좌표계: 프레임버퍼 픽셀, 왼쪽 위 원점
static glm::vec2 cursorFb(GLFWwindow *win, double x, double y) {
  int ww, wh, fw, fh;
  glfwGetWindowSize(win, &ww, &wh);
  glfwGetFramebufferSize(win, &fw, &fh);
  float sx = ww > 0 ? (float)fw / ww : 1.0f;
  float sy = wh > 0 ? (float)fh / wh : 1.0f;
  return glm::vec2((float)x * sx, (float)y * sy);
}

static void onMouseButton(GLFWwindow *win, int button, int action, int) {
  if (button != GLFW_MOUSE_BUTTON_LEFT)
    return;
  App &a = *(App *)glfwGetWindowUserPointer(win);
  double x, y;
  glfwGetCursorPos(win, &x, &y);
  glm::vec2 p = cursorFb(win, x, y);
  InputType t =
      action == GLFW_PRESS ? InputType::PointerDown : InputType::PointerUp;
  a.inputEvents.push_back({t, 0, p.x, p.y, 0});
}

static void onCursorPos(GLFWwindow *win, double x, double y) {
  // 안드로이드처럼 누르고 있는 동안만 MOVE
  if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
    return;
  App &a = *(App *)glfwGetWindowUserPointer(win);
  glm::vec2 p = cursorFb(win, x, y);
  a.inputEvents.push_back({InputType::PointerMove, 0, p.x, p.y, 0});
}

static void onKey(GLFWwindow *win, int key, int, int action, int) {
  if (action != GLFW_PRESS)
    return;
  App &a = *(App *)glfwGetWindowUserPointer(win);
  if (key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(win, GLFW_TRUE);
  if (key == GLFW_KEY_F5) {
    // 셰이더 리로드 (컴파일 실패하면 이전 셰이더 유지)
    a.shaders.recreateAll();
    LOG("shader reloaded");
  }
}

int main() {
  App a;

  a.shaders.add("main", VsDir, FsDir);
  a.grid.shaderRegister(a.shaders);
  a.grid.setPlane(1); // XY 격자
  a.arrow2D.shaderRegister(a.shaders);
  a.font.load("fonts/pixel.ttf", 32.0f);

  if (!initGL(a)) {
    termGL(a);
    return -1;
  }
  glfwSetWindowUserPointer(a.win, &a);
  glfwSetKeyCallback(a.win, onKey);
  glfwSetMouseButtonCallback(a.win, onMouseButton);
  glfwSetCursorPosCallback(a.win, onCursorPos);

  loadLuaScript(a, DEFAULT_SCRIPT);
  pollScriptFile(a, true);
  g_listenFd = openServer();

  double prev = glfwGetTime();
  while (!glfwWindowShouldClose(a.win)) {
    glfwPollEvents();

    double t = glfwGetTime();
    float dt = (float)(t - prev);
    prev = t;
    if (dt > 0.1f)
      dt = 0.1f; // 창 드래그 등으로 멈춘 직후 튐 방지

    pollSocket(a);
    pollScriptFile(a);
    frame(a, dt);
  }

  termGL(a);
  if (g_listenFd != (sock_t)-1)
    closesock(g_listenFd);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}
