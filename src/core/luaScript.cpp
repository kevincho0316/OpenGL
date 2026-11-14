#include "core/luaScript.h"
#include "core/App.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <stdexcept>
#include <string>

// THIS IS ONLY TEMP
// I WILL MAKE A OBJECT CLASS
void setTrans(App &a, const std::string &sn, float angle, glm::vec3 angleVec,
              glm::vec3 scale) {
  glm::mat4 trans = glm::mat4(1.0f);
  trans = glm::rotate(trans, glm::radians(angle), angleVec);
  trans = glm::scale(trans, scale);

  ShaderEntry *e = a.shaders.get(sn);
  if (!e)
    throw std::runtime_error("unknown shader: " + sn);

  e->params["transform"] = trans; // glUniformMatrix4fv 대신
}

// ---------------- Lua ----------------
// 성공 시 "ok", 실패 시 에러 메시지. 실패하면 이전 스크립트 유지.
std::string loadLuaScript(App &a, const std::string &src) {
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

  L->set_function("setBool",
                  [&a](const std::string &sn, const std::string &n, bool v) {
                    ShaderEntry *e = a.shaders.get(sn);
                    if (!e)
                      throw std::runtime_error("unknown shader: " + sn);
                    e->params[n] = v;
                  });

  L->set_function("setInt",
                  [&a](const std::string &sn, const std::string &n, int v) {
                    ShaderEntry *e = a.shaders.get(sn);
                    if (!e)
                      throw std::runtime_error("unknown shader: " + sn);
                    e->params[n] = v;
                  });

  L->set_function("setFloat",
                  [&a](const std::string &sn, const std::string &n, float v) {
                    ShaderEntry *e = a.shaders.get(sn);
                    if (!e)
                      throw std::runtime_error("unknown shader: " + sn);
                    e->params[n] = v;
                  });

  L->set_function("setVec3", [&a](const std::string &sn, const std::string &n,
                                  glm::vec3 v) {
    ShaderEntry *e = a.shaders.get(sn);
    if (!e)
      throw std::runtime_error("unknown shader: " + sn);
    e->params[n] = v;
  });

  L->set_function("setVec4", [&a](const std::string &sn, const std::string &n,
                                  glm::vec4 v) {
    ShaderEntry *e = a.shaders.get(sn);
    if (!e)
      throw std::runtime_error("unknown shader: " + sn);
    e->params[n] = v;
  });

  L->set_function("setTrans",
                  [&a](float angle, glm::vec3 axis, glm::vec3 scale) {
                    setTrans(a, "main", angle, axis, scale);
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

  //-------text----------
  L->set_function("setFont", [&a](const std::string &p, float size) {
    if (!a.font.load(p.c_str(), size))
      return;                    // 실패 시 기존 텍스처 유지
    if (a.glReady()) // 컨텍스트가 있으면 바로 텍스처 갱신
      a.textRenderer.init(a.font);
  });

  L->set_function("setText", [&a](std::string s, float x, float y, float size,
                                  glm::vec4 color) {
    a.texts.push_back({s, x, y, size, color});
  });

  //-------Rect----------
  L->set_function("rect",
                  [&a](const std::string &sn, glm::vec4 rect, glm::vec3 color) {
                    ShaderEntry *s = a.shaders.get(sn);
                    if (!s->shader.ID)
                      return;

                    a.renderer.submit({Pass::Transparent,
                                       s,
                                       a.vao,
                                       GL_TRIANGLE_STRIP,
                                       0,
                                       4,
                                       {{"uRect", rect}, {"uColor", color}}});
                  });

  L->set_function("print", [&a](sol::this_state ts, sol::variadic_args va) {
    sol::state_view lv(ts);
    sol::protected_function tostr = lv["tostring"];
    std::string line;
    for (auto v : va) {
      std::string s = tostr(v);
      line += s + "\t";
    }
    a.printBuf += line + "\n";
    if (a.printBuf.size() > 4096)
      a.printBuf.erase(0, a.printBuf.size() - 4096);
  });

  auto r = L->safe_script(src, sol::script_pass_on_error);
  if (!r.valid()) {
    sol::error e = r;
    return std::string("load error: ") + e.what();
  }
  a.lua = std::move(L);
  a.lastError.clear();
  a.needInit = true;
  return "ok";
}
