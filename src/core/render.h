// src/core/render.h
#pragma once
#include "core/shader.h"
#include "core/shaderLib.h"
#include "core/uniform.h"
#include "gl.h"
#include <vector>

// Emissive: HDR premultiplied-alpha (GL_ONE, GL_ONE_MINUS_SRC_ALPHA), depth
// test on, depth write off. Drawn after Opaque / Transparent.
enum class Pass { Opaque, Transparent, Emissive, Overlay, UI };

struct DrawCmd {
    Pass pass = Pass::Opaque;

    const ShaderEntry *shaderEntry = nullptr;
    GLuint vao = 0;
    GLenum mode = GL_TRIANGLES;
    GLint first = 0;
    GLsizei count = 0;

    std::vector<ShaderUniform> uniforms; // 이 그리기 전용
    GLsizei instances = 0; // > 0 = glDrawArraysInstanced
};

class Renderer {
  private:
    std::vector<DrawCmd> queue_;
    static void setPassState(Pass p);

  public:
    std::vector<ShaderUniform> frameUniforms; // view, projection 등 공통값

    void submit(DrawCmd cmd) { queue_.push_back(std::move(cmd)); }
    void flush() { flush(Pass::UI); }
    // Draws queued commands with pass <= last; later passes stay queued
    // (scene passes -> post-processing -> Overlay / UI on the final image)
    void flush(Pass last);
};
