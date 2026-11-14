#pragma once

#include "core/arrow2D.h"
#include "gl.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#ifdef __ANDROID__
#include <EGL/egl.h>
#else
struct GLFWwindow;
#endif

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "core/Grid.h"
#include "core/camera.h"
#include "core/font.h"
#include "core/input.h"
#include "core/render.h"
#include "core/shader.h"
#include "core/shaderLib.h"
#include "core/textRenderer.h"

struct App {
#ifdef __ANDROID__
  // EGL / GL (창이 없어지면 전부 해제 → 다시 생성)
  EGLDisplay dpy = EGL_NO_DISPLAY;
  EGLSurface surf = EGL_NO_SURFACE;
  EGLContext ctx = EGL_NO_CONTEXT;
  bool glReady() const { return ctx != EGL_NO_CONTEXT; }
#else
  GLFWwindow *win = nullptr;
  bool glReady() const { return win != nullptr; }
#endif
  GLuint vao = 0, vbo = 0;
  float clear[3] = {0, 0, 0};
  float t = 0;
  ShaderLib shaders;
  Camera cam; // 스크립트 리로드해도 유지됨
  Grid grid;
  Font font;
  TextRenderer textRenderer;

  std::vector<TextCmd> texts;
  std::vector<inputEvent> inputEvents;
  Renderer renderer;

  Arrow2D arrow2D;
  bool dragging = false;
  glm::vec2 dragStart{0.0f}, dragCur{0.0f};

  // Lua (GL 객체를 갖지 않으므로 컨텍스트 손실과 무관하게 유지)
  std::unique_ptr<sol::state> lua;
  bool needInit = false;
  std::string lastError; // 런타임 에러 (status 로 조회)
  std::string printBuf;  // print() 출력 (status 로 조회)
#ifdef __ANDROID__
  int listenFd = -1; // 데스크톱은 main.cpp 에서 소켓 관리 (윈도우 SOCKET 타입)
#endif
};
