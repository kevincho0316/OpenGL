#include "Grid.h"
#include "../gl.h"
#include "camera.h"
#include "shader.h"
#include "sol/traits.hpp"
#include <glm/glm.hpp>

bool Grid::init() {
    float quad[] = {
        -1, 0, -1, 1, 0, -1, 1, 0, 1, -1, 0, -1, 1, 0, 1, -1, 0, 1,
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

void Grid::destroy() {}

void Grid::draw(const Camera &cam, float aspect) {
    shader_.use();
    shader_.setMat4("uViewProj", cam.projection(aspect));
    shader_.setVec2("uCamXZ", glm::vec2(cam.position.x, cam.position.z));
    shader_.setFloat("uScale", scale);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
