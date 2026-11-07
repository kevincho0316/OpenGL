#include "textRenderer.h"
#include "core/shader.h"
#include <GLES3/gl3.h>
#include <glm/gtc/matrix_transform.hpp>

bool TextRenderer::init(const Font &font) {
    destroy(); // 중복 호출 대비
    font_ = &font;
    shader_ = Shader("shaders/text.vert", "shaders/text.frag");

    // 아틀라스 -> 텍스처 (1바이트 알파)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, font.atlasWidth(), font.atlasHeight(),
                 0, GL_RED, GL_UNSIGNED_BYTE, font.atlasData());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);

    // 정점 레이아웃: (x, y, u, v)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glBindVertexArray(
        0); // VAO를 먼저 닫는다 (EBO를 먼저 unbind하면 VAO에서 풀림)
    return true;
}

void TextRenderer::destroy() {
    shader_.destroy();
    if (tex_) {
        glDeleteTextures(1, &tex_);
        tex_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (ebo_) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

void TextRenderer::draw(const char *text, float x, float y, float scale,
                        const glm::vec4 &color, int screenW, int screenH) {
    if (!font_ || !tex_)
        return;

    verts_.clear();
    indices_.clear();
    unsigned int base = 0;

    float cx = 0.0f, cy = 0.0f;
    for (const char *p = text; *p; ++p) {
        stbtt_aligned_quad q;
        if (!font_->getQuad(*p, cx, cy, q))
            continue;
        float x0 = x + q.x0 * scale, x1 = x + q.x1 * scale;
        float y0 = y + q.y0 * scale, y1 = y + q.y1 * scale;

        const float v[] = {
            x0, y0, q.s0, q.t0, x1, y0, q.s1, q.t0,
            x1, y1, q.s1, q.t1, x0, y1, q.s0, q.t1,
        };
        verts_.insert(verts_.end(), v, v + 16);

        const unsigned int idx[] = {base, base + 1, base + 2,
                                    base, base + 2, base + 3};
        indices_.insert(indices_.end(), idx, idx + 6);
        base += 4;
    }

    if (indices_.empty())
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_.use();
    shader_.setMat4("uProj",
                    glm::ortho(0.0f, (float)screenW, (float)screenH, 0.0f));
    shader_.setVec4("uColor", color);
    shader_.setInt("uTex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, verts_.size() * sizeof(float), verts_.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices_.size() * sizeof(unsigned int), indices_.data(),
                 GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices_.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
