#include "core/physics.h"

static glm::vec3 stepVelocity(RigidBody &rb, float dt,
                              const glm::vec3 &gravity) {
    if (rb.invMass == 0.0f) { // 고정 물체: 안 움직임
        rb.force = glm::vec3(0.0f);
        return glm::vec3(0.0f);
    }
    glm::vec3 accel = rb.force * rb.invMass + gravity * rb.gravityScale;
    rb.velocity += accel * dt;  // 1) 속도 먼저
    rb.force = glm::vec3(0.0f); // 이번 프레임 힘은 다 썼음
    return rb.velocity * dt;    // 2) 새 속도로 이동량 계산
}

void integrate(RigidBody &rb, Transform2D &t, float dt,
               const glm::vec3 &gravity) {
    glm::vec3 d = stepVelocity(rb, dt, gravity);
    t.position_ += glm::vec2(d); // xy만 사용
}

void integrate(RigidBody &rb, Transform3D &t, float dt,
               const glm::vec3 &gravity) {
    t.position_ += stepVelocity(rb, dt, gravity);
}
