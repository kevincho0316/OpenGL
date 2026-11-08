#pragma once

#include "../gl.h"
#include "camera.h"
#include "shader.h"
#include <glm/glm.hpp>

class Grid {
  private:
    float scale = 500.0f;
    GLuint vao_ = 0, vbo_ = 0;
    Shader shader_;
    int plane = 0;

  public:
    Grid() = default;
    ~Grid() { destroy(); }

    Grid(const Grid &) = delete;
    Grid &operator=(const Grid &) = delete;
    bool init();
    void destroy();

    void draw(const Camera &cam, float aspect);

    void setPlane(int p) { plane = p; }
    void setScale(float s) { scale = s; }
};
