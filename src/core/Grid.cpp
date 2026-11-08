#include "Grid.h"
#include "../gl.h"
#include "camera.h"
#include "shader.h"
#include "sol/traits.hpp"
#include <glm/glm.hpp>

bool Grid::init() {
    float quad[] = {
        -1, -1, 0, 1, -1, 0, 1, 1, 0, -1, -1, 0, 1, 1, 0, -1, 1, 0,
    };

    shader_ = Shader("shaders/Grid.vert", "shaders/Grid.frag");
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
    shader_.destroy();
    vao_ = vbo_ = 0;
}

void Grid::draw(const Camera &cam, float aspect) {
    shader_.use();
    shader_.setMat4("uViewProj", cam.projection(aspect) * cam.view());
    shader_.setFloat("uScale", scale);
    shader_.setVec3("uCam", cam.position);
    shader_.setInt("uPlane", plane);
    glEnable(GL_BLEND);
    glBindVertexArray(vao_);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}
