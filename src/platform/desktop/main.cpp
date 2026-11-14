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
#include "core/appFrame.h"
#include "core/hotReload.h"
#include "core/log.h"
#include "core/luaScript.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

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
    LOGE("FBO incomplete");
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
    LOGE("Failed to create GLFW window");
    return false;
  }
  glfwMakeContextCurrent(a.win);
  glfwSwapInterval(1);
  if (!GL_LOAD(glfwGetProcAddress)) {
    LOGE("Failed to init GLAD");
    return false;
  }

  appInitGL(a);

  LOGI("GL init ok: %s / %s", (const char *)glGetString(GL_VERSION),
       (const char *)glGetString(GL_RENDERER));
  return true;
}

static void termGL(App &a) {
  if (a.win) {
    destroyFbo();
    appTermGL(a);
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
  LOGI("%s: %s", SCRIPT_PATH, r.c_str());
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

  appFrame(a, dt, w, h);

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

// GLFW 키 코드 → char. 출력 가능한 ASCII 만 (A-Z 는 대문자, GLFW 와 동일)
static char keyChar(int key) {
  if (key >= 32 && key <= 126)
    return (char)key;
  switch (key) {
  case GLFW_KEY_ENTER:
    return '\n';
  case GLFW_KEY_TAB:
    return '\t';
  case GLFW_KEY_BACKSPACE:
    return '\b';
  default:
    return 0;
  }
}

static void onKey(GLFWwindow *win, int key, int, int action, int) {
  App &a = *(App *)glfwGetWindowUserPointer(win);

  if (action == GLFW_PRESS || action == GLFW_RELEASE) {
    char c = keyChar(key);
    if (c)
      a.inputEvents.push_back({action == GLFW_PRESS ? InputType::KeyDown
                                                    : InputType::KeyUp,
                               0, 0.0f, 0.0f, c});
  }

  if (action != GLFW_PRESS)
    return;
  if (key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(win, GLFW_TRUE);
  if (key == GLFW_KEY_F5) {
    // 셰이더 리로드 (main + grid + arrow2D, 실패하면 이전 셰이더 유지)
    a.shaders.recreateAll();
    LOGI("shader reloaded");
  }
}

int main() {
  // 파이프/리다이렉트 시에도 로그 바로 보이게 (log.sh, IDE 콘솔)
  setvbuf(stdout, nullptr, _IONBF, 0);
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
  hotReloadOpen(a, PORT);

  double prev = glfwGetTime();
  while (!glfwWindowShouldClose(a.win)) {
    glfwPollEvents();

    double t = glfwGetTime();
    float dt = (float)(t - prev);
    prev = t;
    if (dt > 0.1f)
      dt = 0.1f; // 창 드래그 등으로 멈춘 직후 튐 방지

    hotReloadPoll(a);
    pollScriptFile(a);
    frame(a, dt);
  }

  termGL(a);
  hotReloadClose(a);
  return 0;
}
