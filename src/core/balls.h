#pragma once
// 클릭으로 생성되는 공 두 종류. 둘 다 월드 좌표에서 물리 적용,
// 생성 지점에서 maxDist 이상 멀어지면 사라짐.
//   Balls   : Transform3D. 카메라에서 클릭 광선 방향으로 발사
//   Balls2D : Transform2D. 월드 평면 z = planeZ 위에서만 움직임 (화면 고정 X)
#include "core/object.h"
#include "core/render.h"
#include "core/shaderLib.h"
#include "gl.h"
#include <glm/glm.hpp>
#include <optional>
#include <vector>

struct Ray {
    glm::vec3 origin, dir; // dir 은 정규화됨
};
// 화면 좌표(px, 왼쪽 위 원점)를 지나는 카메라 광선
Ray screenRay(glm::vec2 px, glm::vec2 screen, const glm::mat4 &view,
              const glm::mat4 &proj);

class Balls {
  public:
    glm::vec3 gravity{0.0f, -9.8f, 0.0f}; // 월드 단위/s^2
    float radius = 0.3f;                  // 월드 단위
    float maxDist = 30.0f;                // 생성 지점에서 이 거리면 제거
    float launchSpeed = 8.0f;             // 그냥 클릭했을 때 속도
    float dragPower = 0.03f;              // 드래그 1px 당 추가 속도
    float spawnOffset = 1.0f;             // 카메라 앞 이 거리에서 생성
    glm::vec4 color{1.0f, 0.55f, 0.15f, 1.0f};

    Balls() = default;
    ~Balls() { destroy(); }

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    // pos = 생성 위치, vel = 초기 속도 (월드)
    void spawn(const glm::vec3 &pos, const glm::vec3 &vel);
    // 화면 좌표를 지나는 카메라 광선 방향으로 발사
    void launchFromScreen(glm::vec2 px, glm::vec2 screen,
                          const glm::mat4 &view, const glm::mat4 &proj,
                          float dragLen);
    void update(float dt); // 적분 + 멀어진 공 제거
    void draw(Renderer &r) const;

    size_t count() const { return balls_.size(); }

  private:
    struct Ball {
        object obj;
        glm::vec3 origin;
    };
    std::vector<Ball> balls_;

    const ShaderEntry *shaderEntry_ = nullptr;
    GLuint vao_ = 0, vbo_ = 0;
};

class Balls2D {
  public:
    float planeZ = 0.01f;                  // 공이 사는 월드 평면 z
    glm::vec3 gravity{0.0f, -9.8f, 0.0f}; // xy 만 사용
    float radius = 0.3f;
    float maxDist = 30.0f;
    float launchScale = 3.0f; // 드래그한 평면 거리 → 속도 배율
    glm::vec4 color{0.2f, 0.7f, 1.0f, 1.0f};

    Balls2D() = default;
    ~Balls2D() { destroy(); }

    bool shaderRegister(ShaderLib &lib); // Balls 와 같은 셰이더 공유
    bool init(ShaderLib &lib);
    void destroy();

    void spawn(glm::vec2 pos, glm::vec2 vel);
    // 클릭 광선이 평면 z = planeZ 와 만나는 점 (뒤쪽/평행이면 없음)
    std::optional<glm::vec2> hitPlane(glm::vec2 px, glm::vec2 screen,
                                      const glm::mat4 &view,
                                      const glm::mat4 &proj) const;
    // 누른 곳에서 생성, 드래그 끝까지의 평면 거리만큼 발사. 평면 못 맞추면 무시
    void launchFromScreen(glm::vec2 downPx, glm::vec2 upPx, glm::vec2 screen,
                          const glm::mat4 &view, const glm::mat4 &proj);
    void update(float dt);
    void draw(Renderer &r) const;

    size_t count() const { return balls_.size(); }

  private:
    struct Ball {
        object obj;
        glm::vec2 origin;
    };
    std::vector<Ball> balls_;

    const ShaderEntry *shaderEntry_ = nullptr;
    GLuint vao_ = 0, vbo_ = 0;
};
