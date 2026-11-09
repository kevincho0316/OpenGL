#include "arrow2D.h"
#include "glm/fwd.hpp"
#include "platform/android/log.h"
bool Arrow2D::init() {
    shader_ = Shader("shaders/arrow2D.vert", "shaders/singleColor.frag");

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

void Arrow2D::draw(glm::vec2 an, glm::vec2 dir, float s, glm::vec2 screen) {
    anchor_ = an;
    dir_ = dir;
    size_ = s;
    screen_ = screen;
    shader_.use();
    LOGI("A use=0x%x", glGetError());

    shader_.setVec2("uScreen", screen_); // 화면 크기 (픽셀)
    shader_.setVec2("uAnchor",
                    anchor_);      // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
    shader_.setVec2("uDir", dir_); // 화살표 꼬리 위치 (픽셀, 왼쪽 위 원점)
    shader_.setFloat("uSize", size_);

    shader_.setVec4("uSingleColor", color_);
    LOGI("B uniforms=0x%x", glGetError());

    glEnable(GL_BLEND);
    glBindVertexArray(vao_);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 9);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
    LOGI("C draw=0x%x", glGetError());
}

void Arrow2D::destroy() {
    shader_.destroy();
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}
