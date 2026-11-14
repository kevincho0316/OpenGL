#include "core/balls.h"
#include "core/log.h"
#include "core/physics.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp> // unProject, translate

// ---- 공용 ----

static const char *kShaderName = "Ball";

static bool registerBallShader(ShaderLib &lib) {
    // 이미 있으면 false 지만 문제 없음 (Balls / Balls2D 공유)
    return lib.add(kShaderName, "shaders/ball.vert", "shaders/ball.frag");
}

static void makeQuad(GLuint &vao, GLuint &vbo) {
    const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);
    glBindVertexArray(0);
}

static void freeQuad(GLuint &vao, GLuint &vbo) {
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

// 마지막 20% 구간에서 서서히 사라짐
static glm::vec4 fadeColor(glm::vec4 c, float dist, float maxDist) {
    float k = dist / maxDist;
    c.a *= glm::clamp((1.0f - k) / 0.2f, 0.0f, 1.0f);
    return c;
}

static void submitBall(Renderer &r, const ShaderEntry *sh, GLuint vao,
                       const glm::mat4 &model, const glm::vec4 &color) {
    r.submit({Pass::Transparent,
              sh,
              vao,
              GL_TRIANGLE_STRIP,
              0,
              4,
              {{"uModel", model}, {"uColor", color}}});
}

Ray screenRay(glm::vec2 px, glm::vec2 screen, const glm::mat4 &view,
              const glm::mat4 &proj) {
    glm::vec4 vp(0.0f, 0.0f, screen.x, screen.y);
    glm::vec3 win(px.x, screen.y - px.y, 0.0f); // GL 은 y 위로 증가
    glm::vec3 nearP = glm::unProject(win, view, proj, vp);
    win.z = 1.0f;
    glm::vec3 farP = glm::unProject(win, view, proj, vp);
    return {nearP, glm::normalize(farP - nearP)};
}

// ---- Balls (Transform3D) ----

bool Balls::shaderRegister(ShaderLib &lib) { return registerBallShader(lib); }

bool Balls::init(ShaderLib &lib) {
    shaderEntry_ = lib.get(kShaderName);
    if (shaderEntry_ == nullptr) {
        LOGE("Balls::NEED to register the shader to shader lib");
        return false;
    }
    makeQuad(vao_, vbo_);
    return true;
}

void Balls::destroy() { freeQuad(vao_, vbo_); }

void Balls::spawn(const glm::vec3 &pos, const glm::vec3 &vel) {
    Ball b;
    Transform3D t;
    t.position_ = pos;
    t.scale_ = glm::vec3(radius); // 셰이더가 scale 을 반지름으로 사용
    b.obj.transform = t;
    RigidBody rb;
    rb.velocity = vel;
    b.obj.rigidbody = rb;
    b.origin = pos;
    balls_.push_back(b);
}

void Balls::launchFromScreen(glm::vec2 px, glm::vec2 screen,
                             const glm::mat4 &view, const glm::mat4 &proj,
                             float dragLen) {
    Ray ray = screenRay(px, screen, view, proj);
    float speed = launchSpeed + dragLen * dragPower;
    spawn(ray.origin + ray.dir * spawnOffset, ray.dir * speed);
}

void Balls::update(float dt) {
    for (Ball &b : balls_)
        integrate(*b.obj.rigidbody, std::get<Transform3D>(b.obj.transform), dt,
                  gravity);

    balls_.erase(std::remove_if(balls_.begin(), balls_.end(),
                                [&](const Ball &b) {
                                    const glm::vec3 &p =
                                        std::get<Transform3D>(b.obj.transform)
                                            .position_;
                                    return glm::length(p - b.origin) >= maxDist;
                                }),
                 balls_.end());
}

void Balls::draw(Renderer &r) const {
    if (!vao_)
        return;
    for (const Ball &b : balls_) {
        const glm::vec3 &p = std::get<Transform3D>(b.obj.transform).position_;
        submitBall(r, shaderEntry_, vao_, b.obj.model(),
                   fadeColor(color, glm::length(p - b.origin), maxDist));
    }
}

// ---- Balls2D (Transform2D, 평면 z = planeZ) ----

bool Balls2D::shaderRegister(ShaderLib &lib) { return registerBallShader(lib); }

bool Balls2D::init(ShaderLib &lib) {
    shaderEntry_ = lib.get(kShaderName);
    if (shaderEntry_ == nullptr) {
        LOGE("Balls2D::NEED to register the shader to shader lib");
        return false;
    }
    makeQuad(vao_, vbo_);
    return true;
}

void Balls2D::destroy() { freeQuad(vao_, vbo_); }

void Balls2D::spawn(glm::vec2 pos, glm::vec2 vel) {
    Ball b;
    Transform2D t;
    t.position_ = pos;
    t.scale_ = glm::vec2(radius);
    b.obj.transform = t;
    RigidBody rb;
    rb.velocity = glm::vec3(vel, 0.0f);
    b.obj.rigidbody = rb;
    b.origin = pos;
    balls_.push_back(b);
}

std::optional<glm::vec2> Balls2D::hitPlane(glm::vec2 px, glm::vec2 screen,
                                           const glm::mat4 &view,
                                           const glm::mat4 &proj) const {
    Ray ray = screenRay(px, screen, view, proj);
    if (std::fabs(ray.dir.z) < 1e-6f) // 평면과 평행
        return std::nullopt;
    float t = (planeZ - ray.origin.z) / ray.dir.z;
    if (t < 0.0f) // 평면이 카메라 뒤
        return std::nullopt;
    glm::vec3 p = ray.origin + ray.dir * t;
    return glm::vec2(p);
}

void Balls2D::launchFromScreen(glm::vec2 downPx, glm::vec2 upPx,
                               glm::vec2 screen, const glm::mat4 &view,
                               const glm::mat4 &proj) {
    auto start = hitPlane(downPx, screen, view, proj);
    if (!start)
        return;
    glm::vec2 vel(0.0f);
    if (auto end = hitPlane(upPx, screen, view, proj))
        vel = (*end - *start) * launchScale;
    spawn(*start, vel);
}

void Balls2D::update(float dt) {
    for (Ball &b : balls_)
        integrate(*b.obj.rigidbody, std::get<Transform2D>(b.obj.transform), dt,
                  gravity);

    balls_.erase(std::remove_if(balls_.begin(), balls_.end(),
                                [&](const Ball &b) {
                                    glm::vec2 p = std::get<Transform2D>(
                                                      b.obj.transform)
                                                      .position_;
                                    return glm::length(p - b.origin) >= maxDist;
                                }),
                 balls_.end());
}

void Balls2D::draw(Renderer &r) const {
    if (!vao_)
        return;
    // Transform2D::matrix() 는 z = 0 에 놓으므로 평면 위치로 옮김
    glm::mat4 toPlane =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, planeZ));
    for (const Ball &b : balls_) {
        glm::vec2 p = std::get<Transform2D>(b.obj.transform).position_;
        submitBall(r, shaderEntry_, vao_, toPlane * b.obj.model(),
                   fadeColor(color, glm::length(p - b.origin), maxDist));
    }
}
