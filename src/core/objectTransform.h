#pragma once
#include "gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float3.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp> // defines glm::quat, plus mat4_cast, angleAxis, etc.

struct Transform2D {
    glm::vec2 position_{0.0f};
    float rotation_{0.0f}; // radian Z axis
    glm::vec2 scale_{1.0f};
    glm::mat4 matrix() const; // model transfer
};

struct Transform3D {
    glm::vec3 position_{0.0f};
    glm::quat rotation_{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale_{1.0f};
    glm::mat4 matrix() const; // Linear transfer
};
