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
  a.balls.init(a.shaders);
  a.planeBalls.init(a.shaders);
  a.modeBtn.init(a.shaders);
  a.textRenderer.init(a.font);
}

void appTermGL(App &a) {
  a.textRenderer.destroy();
  a.grid.destroy();
  a.arrow2D.destroy();
  a.balls.destroy();
  a.planeBalls.destroy();
  a.modeBtn.destroy();
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

static void handleInput(App &a, glm::vec2 screen, const glm::mat4 &view,
                        const glm::mat4 &proj) {
  for (const inputEvent &i : a.inputEvents) {
    switch (i.inputType) {
    case InputType::PointerDown:
      if (a.modeBtn.pointerDown(glm::vec2(i.x, i.y)))
        break; // 버튼이 먹음: 공 생성 / 화살표 없음
      a.dragging = true;
      a.dragStart = a.dragCur = glm::vec2(i.x, i.y);
      break;
    case InputType::PointerMove:
      a.dragCur = glm::vec2(i.x, i.y);
      break;
    case InputType::PointerUp:
      if (a.modeBtn.pointerUp(glm::vec2(i.x, i.y)))
        a.usePlaneBalls = !a.usePlaneBalls;
      if (a.dragging) {
        a.dragCur = glm::vec2(i.x, i.y);
        if (a.usePlaneBalls) {
          // 누른 곳의 평면 위치에서 생성, 드래그 끝까지 = 발사 속도
          a.planeBalls.launchFromScreen(a.dragStart, a.dragCur, screen, view,
                                        proj);
        } else {
          // 누른 곳을 지나는 카메라 광선으로 발사. 드래그 길이 = 추가 힘
          float dragLen = glm::length(a.dragCur - a.dragStart);
          a.balls.launchFromScreen(a.dragStart, screen, view, proj, dragLen);
        }
      }
      a.dragging = false;
      break;
    case InputType::KeyDown:
      if (i.key == '1')
        a.usePlaneBalls = false;
      else if (i.key == '2')
        a.usePlaneBalls = true;
      break;
    default:
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

  glm::mat4 view = a.cam.view();
  glm::mat4 proj = a.cam.projection(aspect);
  a.renderer.frameUniforms = {
      {"view", view},
      {"projection", proj},
      {"uTime", a.t},
  };

  // 버튼 배치: 화면 높이에 비례 (폰에서도 누르기 쉽게)
  float btnH = glm::max(60.0f, (float)h * 0.07f);
  a.modeBtn.size = glm::vec2(btnH * 5.0f, btnH);
  a.modeBtn.pos = glm::vec2(btnH * 0.3f);

  handleInput(a, glm::vec2(w, h), view, proj);
  runLua(a, dt);

  a.balls.update(dt);
  a.balls.draw(a.renderer);
  a.planeBalls.update(dt);
  a.planeBalls.draw(a.renderer);
  a.modeBtn.label = a.usePlaneBalls ? "Plane ball (2D)" : "3D ball";
  a.modeBtn.color = a.usePlaneBalls ? glm::vec4(0.1f, 0.35f, 0.55f, 0.85f)
                                    : glm::vec4(0.55f, 0.3f, 0.1f, 0.85f);
  a.modeBtn.draw(a.renderer, a.texts, a.font, glm::vec2(w, h));

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
