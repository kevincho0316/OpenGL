#include "core/objectTransform.h"
#include "glm/ext/matrix_float4x4.hpp"
#include <glm/gtc/matrix_transform.hpp> // translate, rotate, scale
#include <glm/gtc/quaternion.hpp>       // mat4_cast

glm::mat4 Transform2D::matrix() const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, glm::vec3(position_, 0.0f));
    m = glm::rotate(m, rotation_, glm::vec3(0, 0, 1));
    m = glm::scale(m, glm::vec3(scale_, 1.0f));
    return m;
}

glm::mat4 Transform3D::matrix() const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, position_);  // T
    m = m * glm::mat4_cast(rotation_); // R
    m = glm::scale(m, scale_);         // S
    return m;                          // = T * R * S
}
