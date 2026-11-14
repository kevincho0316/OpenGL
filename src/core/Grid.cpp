#include "Grid.h"
#include "../gl.h"
#include "camera.h"
#include "core/render.h"
#include "core/shaderLib.h"
#include "core/uniform.h"
#include "platform/android/log.h"
#include "shader.h"
#include "sol/traits.hpp"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <vector>

bool Grid::shaderRegister(ShaderLib &lib) {
    return lib.add("Grid", "shaders/Grid.vert", "shaders/Grid.frag");
};

bool Grid::init(ShaderLib &lib) {
    shaderEntry_ = lib.get("Grid");
    if (!shaderEntry_) {
        LOGE("GRID::ShaderEntry is not registered to ShaderLib");
        return false;
    }

    float quad[] = {
        -1, -1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, 1, 1, 0, -1, 1, 0,
    };

    shaderEntry_ = lib.get("Grid");
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0); //

    return true;
};

void Grid::destroy() {
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = 0;
}

void Grid::draw(Renderer &r, const Camera &cam, float aspect) {
    std::vector<ShaderUniform> su = {
        {"uScale", scale_}, {"uCam", cam.position}, {"uPlane", plane_}};
    r.submit({Pass::Transparent, shaderEntry_, vao_, GL_TRIANGLES, 0, 6, su});
}
