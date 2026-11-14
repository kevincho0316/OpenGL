// Unit tests for core/physics (integrate). No framework: run binary, exit code 0 = pass.
#include "core/physics.h"
#include <cmath>
#include <cstdio>

static int g_failed = 0;
static int g_passed = 0;

static bool near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (cond) {                                                            \
            ++g_passed;                                                        \
        } else {                                                               \
            ++g_failed;                                                        \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                      \
    } while (0)

#define CHECK_NEAR(a, b) CHECK(near((a), (b)))

static const glm::vec3 kGravity{0.0f, -10.0f, 0.0f};

// 고정 물체(invMass=0)는 움직이지 않고, 누적된 힘은 지워져야 함
static void test_static_body_does_not_move() {
    std::puts("static body does not move");
    RigidBody rb;
    rb.invMass = 0.0f;
    rb.velocity = {5.0f, 5.0f, 5.0f};
    rb.addForce({100.0f, 0.0f, 0.0f});
    Transform3D t;
    t.position_ = {1.0f, 2.0f, 3.0f};

    integrate(rb, t, 0.1f, kGravity);

    CHECK_NEAR(t.position_.x, 1.0f);
    CHECK_NEAR(t.position_.y, 2.0f);
    CHECK_NEAR(t.position_.z, 3.0f);
    CHECK(rb.force == glm::vec3(0.0f));
}

// 반암시적 오일러: v 먼저 갱신 → 새 v로 위치 갱신
static void test_single_step_semi_implicit() {
    std::puts("single step uses new velocity (semi-implicit Euler)");
    RigidBody rb;
    Transform3D t;
    integrate(rb, t, 1.0f, kGravity);

    CHECK_NEAR(rb.velocity.y, -10.0f);
    CHECK_NEAR(t.position_.y, -10.0f); // explicit Euler would give 0
}

// n 스텝 후 위치 = g*dt^2 * n(n+1)/2
static void test_free_fall_many_steps() {
    std::puts("free fall over many steps");
    RigidBody rb;
    Transform3D t;
    const float dt = 0.01f;
    const int n = 100;
    for (int i = 0; i < n; ++i)
        integrate(rb, t, dt, kGravity);

    float expectedV = kGravity.y * dt * n;
    float expectedY = kGravity.y * dt * dt * n * (n + 1) / 2.0f;
    CHECK_NEAR(rb.velocity.y, expectedV);
    CHECK(near(t.position_.y, expectedY, 1e-3f));
    // 해석해(-5m)와 대략 비슷해야 함
    CHECK(std::fabs(t.position_.y - (-5.0f)) < 0.1f);
}

// 힘은 한 프레임만 적용되고 0으로 리셋
static void test_force_applied_then_cleared() {
    std::puts("force applied once then cleared");
    RigidBody rb;
    rb.invMass = 0.5f; // mass = 2
    rb.gravityScale = 0.0f;
    rb.addForce({4.0f, 0.0f, 0.0f}); // a = F/m = 2
    Transform3D t;

    integrate(rb, t, 1.0f, kGravity);
    CHECK_NEAR(rb.velocity.x, 2.0f);
    CHECK_NEAR(t.position_.x, 2.0f);
    CHECK(rb.force == glm::vec3(0.0f));

    integrate(rb, t, 1.0f, kGravity); // no force: constant velocity
    CHECK_NEAR(rb.velocity.x, 2.0f);
    CHECK_NEAR(t.position_.x, 4.0f);
}

static void test_gravity_scale() {
    std::puts("gravityScale 0 ignores gravity, 2 doubles it");
    RigidBody a;
    a.gravityScale = 0.0f;
    Transform3D ta;
    integrate(a, ta, 1.0f, kGravity);
    CHECK_NEAR(ta.position_.y, 0.0f);

    RigidBody b;
    b.gravityScale = 2.0f;
    Transform3D tb;
    integrate(b, tb, 1.0f, kGravity);
    CHECK_NEAR(b.velocity.y, -20.0f);
}

static void test_impulse_scaled_by_inv_mass() {
    std::puts("impulse scaled by invMass");
    RigidBody rb;
    rb.invMass = 0.25f;
    rb.addImpulse({8.0f, 0.0f, 0.0f});
    CHECK_NEAR(rb.velocity.x, 2.0f);

    RigidBody wall;
    wall.invMass = 0.0f;
    wall.addImpulse({8.0f, 0.0f, 0.0f});
    CHECK(wall.velocity == glm::vec3(0.0f));
}

// 2D는 xy만 사용, z 성분은 무시
static void test_2d_uses_xy_only() {
    std::puts("2D integrate uses xy only");
    RigidBody rb;
    rb.gravityScale = 0.0f;
    rb.velocity = {1.0f, 2.0f, 3.0f};
    Transform2D t;
    t.rotation_ = 0.7f;

    integrate(rb, t, 0.5f, kGravity);
    CHECK_NEAR(t.position_.x, 0.5f);
    CHECK_NEAR(t.position_.y, 1.0f);
    CHECK_NEAR(t.rotation_, 0.7f); // rotation untouched
}

static void test_zero_dt_no_change() {
    std::puts("dt = 0 changes nothing");
    RigidBody rb;
    rb.velocity = {1.0f, 1.0f, 1.0f};
    Transform3D t;
    integrate(rb, t, 0.0f, kGravity);
    CHECK(t.position_ == glm::vec3(0.0f));
    CHECK(rb.velocity == glm::vec3(1.0f));
}

int main() {
    test_static_body_does_not_move();
    test_single_step_semi_implicit();
    test_free_fall_many_steps();
    test_force_applied_then_cleared();
    test_gravity_scale();
    test_impulse_scaled_by_inv_mass();
    test_2d_uses_xy_only();
    test_zero_dt_no_change();

    std::printf("\n%d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
