// Desktop: GLFW + glad + GL 3.3 core + Lua(sol2) 핫 리로드 (안드로이드와 동일
// 구조) 스크립트 전송: ./push.sh  (127.0.0.1:7777, NUL 바이트로 끝) 데스크톱
// 전용: src/core/main.lua 저장하면 자동 리로드, F5 = 셰이더 리로드
#include "gl.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define SOL_ALL_SAFETIES_ON 1
#include "shader.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sol/sol.hpp>
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

function update(dt)
  t = t + dt
  clear_color(0.1 + 0.1 * math.sin(t), 0.2, 0.3)
end

)LUA";

static const char *VsDir = "shaders/VertexShader.glsl";
static const char *FsDir = "shaders/ButterflyFgSh.glsl";

struct App {
  GLFWwindow *win = nullptr;
  GLuint vao = 0, vbo = 0;
  float clear[3] = {0, 0, 0};
  Shader shader;
  std::unique_ptr<sol::state> lua;
  std::string lastError; // 런타임 에러 (status 로 조회)
  std::string printBuf;  // print() 출력 (status 로 조회)
  sock_t listenFd = (sock_t)-1;
  std::filesystem::file_time_type scriptTime{};
};

// ---------------- GL ----------------
static bool initGL(App &a) {
  if (!glfwInit())
    return false;
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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

  a.shader = Shader(VsDir, FsDir);

  const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
  glGenVertexArrays(1, &a.vao);
  glGenBuffers(1, &a.vbo);
  glBindVertexArray(a.vao);
  glBindBuffer(GL_ARRAY_BUFFER, a.vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  LOG("GL init ok: %s", (const char *)glGetString(GL_VERSION));
  return true;
}

static void termGL(App &a) {
  if (a.win) {
    a.shader.destroy();
    glDeleteBuffers(1, &a.vbo);
    glDeleteVertexArrays(1, &a.vao);
    a.vao = a.vbo = 0;
    glfwDestroyWindow(a.win);
    a.win = nullptr;
  }
  glfwTerminate();
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

// ---------------- 파일 감시 (데스크톱 전용) ----------------
// main.lua 수정 시간이 바뀌면 다시 읽어서 리로드
static void pollScriptFile(App &a, bool force = false) {
  std::error_code ec;
  auto t = std::filesystem::last_write_time(SCRIPT_PATH, ec);
  if (ec || (!force && t == a.scriptTime))
    return;
  std::ifstream f(SCRIPT_PATH, std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  std::string src = ss.str();
  if (src.empty())
    return; // 에디터가 저장 도중일 수 있음 → 다음 프레임에 재시도
  a.scriptTime = t;
  loadScript(a, src);
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
  if (a.listenFd == (sock_t)-1)
    return;
  sock_t c = accept(a.listenFd, nullptr, nullptr);
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
    reply = loadScript(a, src);
  }
  reply += "\n";
  send(c, reply.data(), (int)reply.size(), 0);
  closesock(c);
}

// --------------900-- 프레임 ----------------
static void frame(App &a, float dt) {
  int w = 0, h = 0;
  glfwGetFramebufferSize(a.win, &w, &h);
  glViewport(0, 0, w, h);
  glClearColor(a.clear[0], a.clear[1], a.clear[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  a.shader.use();
  glBindVertexArray(a.vao);

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
  glfwSwapBuffers(a.win);
}

static void onKey(GLFWwindow *win, int key, int, int action, int) {
  if (action != GLFW_PRESS)
    return;
  App &a = *(App *)glfwGetWindowUserPointer(win);
  if (key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(win, GLFW_TRUE);
  if (key == GLFW_KEY_F5) {
    // 셰이더 리로드 (컴파일 실패해도 에러 로그만 찍힘)
    a.shader.destroy();
    a.shader = Shader(VsDir, FsDir);
    LOG("shader reloaded");
  }
}

int main() {
  App a;
  if (!initGL(a)) {
    termGL(a);
    return -1;
  }
  glfwSetWindowUserPointer(a.win, &a);
  glfwSetKeyCallback(a.win, onKey);

  loadScript(a, DEFAULT_SCRIPT);
  pollScriptFile(a, true);
  a.listenFd = openServer();

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
  if (a.listenFd != (sock_t)-1)
    closesock(a.listenFd);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}
