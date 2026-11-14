#pragma once
// "Butterfly Specter" particles: CPU particle system drawn as instanced
// camera-facing quads (shaders/Specter.vert + Specter.frag) in Pass::Emissive.
// Shader parameters live in the ShaderLib entry "Specter" (Lua: setFloat /
// setVec3 / setInt). Defaults are seeded in init():
//   uEmissionStrength 15, uChannelOffsetStep 1, uNoiseMul 0.17,
//   uNoiseFreq (5, 3.7, 10), uTimeMul 4, uDebugChannel 0
#include "core/camera.h"
#include "core/render.h"
#include "core/shaderLib.h"
#include "gl.h"
#include <glm/glm.hpp>
#include <random>
#include <vector>

class SpecterParticles {
  public:
    // Quad UV runs -UV_SCALE .. +UV_SCALE (original Blender plane UV range is
    // uncertain; change here or set uvScale at runtime).
    static constexpr float UV_SCALE = 1.0f;

    int count = 40;
    float lifeMin = 1.0f, lifeMax = 2.0f;   // lifetime range, seconds
    float scaleMin = 0.5f, scaleMax = 1.5f; // quad half extent, world units
    float distMin = 4.0f, distMax = 10.0f;  // spawn distance in front of camera
    float spread = 0.9f;                    // fraction of the view used for spawning
    float uvScale = UV_SCALE;
    int debugMode = 0; // 0 = final, 1 = one specter() channel grey, 2 = polar

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    // Ages particles; life reaching 1 respawns in front of the camera
    void update(float dt, const Camera &cam, float aspect);
    void draw(Renderer &r); // uploads instance data + submits one instanced draw

  private:
    struct Particle {
        glm::vec3 position{0.0f};
        float scale = 1.0f;
        float life = 0.0f;     // normalised age 0..1
        float random = 0.0f;   // constant 0..1, chosen at spawn
        float lifetime = 1.0f; // seconds
    };
    struct Instance { // matches the vertex layout (locations 1, 2)
        float x, y, z, scale, life, random;
    };

    void respawn(Particle &p, const Camera &cam, float aspect);
    float rand01() { return dist_(rng_); }

    std::vector<Particle> particles_;
    std::vector<Instance> instances_;
    std::mt19937 rng_{1234567u};
    std::uniform_real_distribution<float> dist_{0.0f, 1.0f};

    const ShaderEntry *shaderEntry_ = nullptr;
    GLuint vao_ = 0, quadVbo_ = 0, instVbo_ = 0;
};
