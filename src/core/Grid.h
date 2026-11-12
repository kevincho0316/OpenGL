#pragma once

#include "../gl.h"
#include "camera.h"
#include "core/shaderLib.h"
#include "render.h"
#include <glm/glm.hpp>

class Grid {
  private:
    const ShaderEntry *shaderEntry_ = nullptr;

    float scale_ = 500.0f;
    GLuint vao_ = 0, vbo_ = 0;

    int plane_ = 0;

  public:
    Grid() = default;
    ~Grid() { destroy(); }

    Grid(const Grid &) = delete;
    Grid &operator=(const Grid &) = delete;

    bool shaderRegister(ShaderLib &lib);
    bool init(ShaderLib &lib);
    void destroy();

    void draw(Renderer &r, const Camera &cam, float aspect);

    void setPlane(int p) { plane_ = p; }
    void setScale(float s) { scale_ = s; }
};
