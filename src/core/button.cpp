#include "core/button.h"
#include "core/log.h"
#include <algorithm>

bool Button::shaderRegister(ShaderLib &lib) {
    return lib.add("Button", "shaders/button.vert", "shaders/button.frag");
}

bool Button::init(ShaderLib &lib) {
    shaderEntry_ = lib.get("Button");
    if (shaderEntry_ == nullptr) {
        LOGE("Button::NEED to register the shader to shader lib");
        return false;
    }

    const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glBindVertexArray(0);
    return true;
}

void Button::destroy() {
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

bool Button::contains(glm::vec2 p) const {
    return p.x >= pos.x && p.x <= pos.x + size.x && p.y >= pos.y &&
           p.y <= pos.y + size.y;
}

bool Button::pointerDown(glm::vec2 p) {
    pressed_ = contains(p);
    return pressed_;
}

bool Button::pointerUp(glm::vec2 p) {
    bool clicked = pressed_ && contains(p);
    pressed_ = false;
    return clicked;
}

void Button::draw(Renderer &r, std::vector<TextCmd> &texts, const Font &font,
                  glm::vec2 screen) const {
    if (vao_) {
        float corner = cornerRadius;
        float border = std::max(2.0f, size.y * 0.04f);
        r.submit({Pass::UI,
                  shaderEntry_,
                  vao_,
                  GL_TRIANGLE_STRIP,
                  0,
                  4,
                  {{"uScreen", screen},
                   {"uPos", pos},
                   {"uSize", size},
                   {"uCorner", corner},
                   {"uBorder", border},
                   {"uColor", pressed_ ? pressedColor : color},
                   {"uBorderColor", borderColor}}});
    }

    // 글자: 버튼 높이의 45% 크기로, 가운데 정렬
    float fontPx = font.pixelHeight();
    if (label.empty() || fontPx <= 0.0f)
        return;
    float scale = size.y * 0.45f / fontPx;
    float w = font.textWidth(label.c_str()) * scale;
    float x = pos.x + (size.x - w) * 0.5f;
    float baseline = pos.y + size.y * 0.5f + fontPx * scale * 0.35f;
    texts.push_back({label, x, baseline, scale, textColor});
}
