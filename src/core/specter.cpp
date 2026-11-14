#include "core/specter.h"
#include "core/log.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

static const char *kShaderName = "Specter";

bool SpecterParticles::shaderRegister(ShaderLib &lib) {
    return lib.add(kShaderName, "shaders/Specter.vert", "shaders/Specter.frag");
}

bool SpecterParticles::init(ShaderLib &lib) {
    ShaderEntry *e = lib.get(kShaderName);
    if (e == nullptr) {
        LOGE("Specter::NEED to register the shader to shader lib");
        return false;
    }
    shaderEntry_ = e;
    // Defaults (try_emplace: values already set from Lua survive a context
    // loss / re-init)
    e->params.try_emplace("uEmissionStrength", 15.0f);
    e->params.try_emplace("uChannelOffsetStep", 1.0f);
    e->params.try_emplace("uNoiseMul", 0.17f);
    e->params.try_emplace("uNoiseFreq", glm::vec3(5.0f, 3.7f, 10.0f));
    e->params.try_emplace("uTimeMul", 4.0f);
    e->params.try_emplace("uDebugChannel", 0);

    const float quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &quadVbo_);
    glGenBuffers(1, &instVbo_);
    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, instVbo_);
    glEnableVertexAttribArray(1); // xyz + scale
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          (void *)offsetof(Instance, x));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2); // life + random
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Instance),
                          (void *)offsetof(Instance, life));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return true;
}

void SpecterParticles::destroy() {
    if (quadVbo_)
        glDeleteBuffers(1, &quadVbo_);
    if (instVbo_)
        glDeleteBuffers(1, &instVbo_);
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    vao_ = quadVbo_ = instVbo_ = 0;
}

void SpecterParticles::respawn(Particle &p, const Camera &cam, float aspect) {
    float d = glm::mix(distMin, distMax, rand01());
    // Half size of the view at distance d
    float halfH = cam.perspective
                      ? d * std::tan(glm::radians(cam.fov) * 0.5f)
                      : cam.orthoSize;
    float halfW = halfH * aspect;
    float rx = (rand01() * 2.0f - 1.0f) * spread;
    float ry = (rand01() * 2.0f - 1.0f) * spread;
    p.position = cam.position + cam.front() * d + cam.right() * (rx * halfW) +
                 cam.up() * (ry * halfH);
    p.scale = glm::mix(scaleMin, scaleMax, rand01());
    p.lifetime = glm::mix(lifeMin, lifeMax, rand01());
    p.random = rand01();
    p.life = 0.0f;
}

void SpecterParticles::update(float dt, const Camera &cam, float aspect) {
    size_t n = (size_t)std::max(count, 0);
    if (particles_.size() > n)
        particles_.resize(n);
    while (particles_.size() < n) {
        Particle p;
        respawn(p, cam, aspect);
        p.life = rand01(); // stagger so they do not all respawn together
        particles_.push_back(p);
    }

    for (Particle &p : particles_) {
        p.life += dt / std::max(p.lifetime, 1e-3f);
        if (p.life >= 1.0f)
            respawn(p, cam, aspect);
    }
}

void SpecterParticles::draw(Renderer &r) {
    if (!vao_ || particles_.empty())
        return;

    instances_.clear();
    for (const Particle &p : particles_)
        instances_.push_back({p.position.x, p.position.y, p.position.z,
                              p.scale, p.life, p.random});

    glBindBuffer(GL_ARRAY_BUFFER, instVbo_);
    glBufferData(GL_ARRAY_BUFFER, instances_.size() * sizeof(Instance),
                 instances_.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    DrawCmd cmd;
    cmd.pass = Pass::Emissive;
    cmd.shaderEntry = shaderEntry_;
    cmd.vao = vao_;
    cmd.mode = GL_TRIANGLE_STRIP;
    cmd.first = 0;
    cmd.count = 4;
    cmd.uniforms = {{"uUvScale", uvScale}, {"uDebugMode", debugMode}};
    cmd.instances = (GLsizei)instances_.size();
    r.submit(std::move(cmd));
}
