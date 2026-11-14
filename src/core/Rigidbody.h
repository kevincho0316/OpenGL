#pragma once
#include <glm/glm.hpp>

struct RigidBody {
    glm::vec3 velocity{0.0f};
    glm::vec3 force{0.0f};    // 프레임마다 누적 → 적분 후 0으로
    float invMass{1.0f};      // 0 = 움직이지 않음 (벽, 바닥)
    float gravityScale{1.0f}; // 0 = 중력 무시

    void addForce(const glm::vec3 &f) { force += f; }
    void addImpulse(const glm::vec3 &j) { velocity += j * invMass; }
};
