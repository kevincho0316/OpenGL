#pragma once
// 화면 고정 UI 버튼 (픽셀 좌표, 왼쪽 위 원점). 마우스 / 터치 둘 다 동작.
// 사용: pointerDown 이 true 를 돌려주면 그 입력은 버튼이 먹은 것 (드래그 시작 X)
//       pointerUp 이 true 면 클릭 완료 (버튼 안에서 눌렀다 뗌)
#include "core/font.h"
#include "core/render.h"
#include "core/shaderLib.h"
#include "core/textRenderer.h"
#include "gl.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Button {
  public:
    glm::vec2 pos{20.0f, 20.0f};   // 왼쪽 위 (픽셀)
    glm::vec2 size{300.0f, 80.0f}; // 크기 (픽셀)
    std::string label;
    float cornerRadius = 0.0f; // 0 = 각진 직사각형, 크면 둥근 모서리 (픽셀)
    glm::vec4 color{0.15f, 0.15f, 0.2f, 0.85f};
    glm::vec4 pressedColor{0.3f, 0.3f, 0.4f, 0.95f};
    glm::vec4 borderColor{1.0f, 1.0f, 1.0f, 0.9f};
    glm::vec4 textColor{1.0f};

    Button() = default;
    ~Button() { destroy(); }

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    bool contains(glm::vec2 p) const;
    bool pointerDown(glm::vec2 p); // 버튼 위면 눌림 상태 + true
    bool pointerUp(glm::vec2 p);   // 눌린 상태에서 버튼 위에서 떼면 true
    bool pressed() const { return pressed_; }

    // 배경은 Renderer(UI 패스)로, 글자는 texts 로 (flush 뒤에 그려짐)
    void draw(Renderer &r, std::vector<TextCmd> &texts, const Font &font,
              glm::vec2 screen) const;

  private:
    bool pressed_ = false;
    const ShaderEntry *shaderEntry_ = nullptr;
    GLuint vao_ = 0, vbo_ = 0;
};
