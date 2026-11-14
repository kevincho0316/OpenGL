#pragma once

#include "../gl.h"
#include "camera.h"
#include "core/render.h"
#include "core/shaderLib.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/fwd.hpp"
#include "shader.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

class Arrow2D {
  private:
    const ShaderEntry *shaderEntry_ = nullptr;

    GLuint vao_ = 0, vbo_ = 0;
    glm::vec2 anchor_;
    glm::vec2 dir_;
    glm::vec2 screen_;
    float size_;
    glm::vec4 color_ = glm::vec4(1.0f);

    float vertices_[18] = {
        // 몸통 (삼각형 2개)
        -0.5f,
        -0.1f,
        0.2f,
        -0.1f,
        0.2f,
        0.1f,
        -0.5f,
        -0.1f,
        0.2f,
        0.1f,
        -0.5f,
        0.1f,
        // 머리 ()
        0.2f,
        -0.3f,
        0.6f,
        0.0f,
        0.2f,
        0.3f,
    };

  public:
    Arrow2D() = default;
    ~Arrow2D() { destroy(); }

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    void draw(Renderer &r, glm::vec2 an, glm::vec2 dir, float s,
              glm::vec2 screen);
};
