#include "arrow2D.h"
#include "core/render.h"
#include "glm/fwd.hpp"
#include "core/log.h"
#include "../gl.h"
#include <chrono>

bool Arrow2D::shaderRegister(ShaderLib &lib) {
    return lib.add("Arrow2D", "shaders/arrow2D.vert",
                   "shaders/stripeColor.frag");
}

bool Arrow2D::init(ShaderLib &lib) {
    shaderEntry_ = lib.get("Arrow2D");

    if (shaderEntry_ == nullptr) {
        LOGE("Arrow2D::NEED to register the shader to shader lib");
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_), vertices_, GL_STATIC_DRAW);
    glBindVertexArray(0);
    return true;
}

void Arrow2D::draw(Renderer &r, glm::vec2 an, glm::vec2 dir, float s,
                   glm::vec2 screen) {
    anchor_ = an;
    dir_ = dir;
    size_ = s;
    screen_ = screen;

    static const auto t0 = std::chrono::steady_clock::now();
    r.submit({
        Pass::Overlay,
        shaderEntry_,
        vao_,
        GL_TRIANGLES,
        0,
        9,
        {{"uScreen", screen_}, // 화면 크기 (픽셀)
         {"uAnchor", anchor_}, // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
         {"uDir", dir_},       // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
         {"uSize", size_},
         {"uStripeColor", color_},
         {"uGap", 45.0f},
         {"uStripeSpeed", 105.0f}},
    });
}

void Arrow2D::destroy() {
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}
