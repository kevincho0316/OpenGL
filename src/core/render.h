// src/core/render.h
#pragma once
#include "core/shader.h"
#include "core/uniform.h"
#include "gl.h"
#include <vector>

enum class Pass { Opaque, Transparent, Overlay, UI };

struct DrawCmd {
    Pass pass = Pass::Opaque;

    const Shader *shader = nullptr;
    GLuint vao = 0;
    GLenum mode = GL_TRIANGLES;
    GLint first = 0;
    GLsizei count = 0;

    std::vector<ShaderUniform> uniforms; // 이 그리기 전용
};

class Renderer {
  private:
    std::vector<DrawCmd> queue_;
    static void setPassState(Pass p);

  public:
    std::vector<ShaderUniform> frameUniforms; // view, projection 등 공통값

    void submit(DrawCmd cmd) { queue_.push_back(std::move(cmd)); }
    void flush(); // 렌더 스레드에서, 프레임마다 1번
};
