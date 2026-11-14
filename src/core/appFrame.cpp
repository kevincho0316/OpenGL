#include "core/appFrame.h"
#include "core/App.h"
#include "core/log.h"
#include <glm/glm.hpp>

void appInitGL(App &a) {
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
}

void appTermGL(App &a) {
  a.textRenderer.destroy();
  a.grid.destroy();
  a.arrow2D.destroy();
  if (a.vbo)
    glDeleteBuffers(1, &a.vbo);
  if (a.vao)
    glDeleteVertexArrays(1, &a.vao);
  a.vao = a.vbo = 0;
  a.shaders.destroyAll();
}

// 빨간 배경 = 에러 (init / update 동일)
static void setLuaError(App &a, const char *where, const char *msg) {
  a.lastError = msg;
  LOGE("%s error: %s", where, msg);
  a.clear[0] = 0.5f;
  a.clear[1] = 0.0f;
  a.clear[2] = 0.0f;
}

static void handleInput(App &a) {
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
    default: // KeyDown / KeyUp: 아직 사용처 없음
      break;
    }
  }
  a.inputEvents.clear();
}

static void runLua(App &a, float dt) {
  if (!a.lua || !a.lastError.empty())
    return;

  if (a.needInit) {
    a.needInit = false;
    sol::protected_function ini = (*a.lua)["init"];
    if (ini.valid()) {
      auto r = ini();
      if (!r.valid()) {
        sol::error e = r;
        setLuaError(a, "init", e.what());
        return;
      }
    }
  }

  sol::protected_function upd = (*a.lua)["update"];
  if (upd.valid()) {
    auto r = upd(dt);
    if (!r.valid()) {
      sol::error e = r;
      setLuaError(a, "update", e.what());
    }
  }
}

void appFrame(App &a, float dt, int w, int h) {
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

  handleInput(a);
  runLua(a, dt);

  glm::vec2 d = a.dragCur - a.dragStart;
  float len = glm::length(d);
  if (a.dragging && len > 1.0f)
    a.arrow2D.draw(a.renderer, a.dragStart, d, len, glm::vec2(w, h));

  a.grid.draw(a.renderer, a.cam, aspect);
  a.renderer.flush();

  for (const TextCmd &t : a.texts)
    a.textRenderer.draw(t.s.c_str(), t.x, t.y, t.size, t.color, w, h);
  a.texts.clear();
}
