#pragma once
#include "../gl.h"
#include "font.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <vector>

class TextRenderer {
  public:
    TextRenderer() = default;
    ~TextRenderer() { destroy(); } // 안전망 (destroy는 중복 호출 OK)

    bool init(const Font &font); // 컨텍스트가 생길 때마다 호출
    void destroy();              // 컨텍스트가 사라지기 전에 호출

    // 화면 좌표(픽셀, 왼쪽 위 원점)의 (x, y) 베이스라인에 글자 그리기
    void draw(const char *text, float x, float y, const glm::vec4 &color,
              int screenW, int screenH);

  private:
    const Font *font_ = nullptr;
    Shader shader_;
    GLuint tex_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    std::vector<float> verts_;          // 글자당 16 floats (정점 4개)
    std::vector<unsigned int> indices_; // 글자당 6개
};
