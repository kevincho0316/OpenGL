#pragma once
#include "core/Rigidbody.h"
#include "core/objectTransform.h"
#include <glm/glm.hpp>

void integrate(RigidBody &rb, Transform2D &t, float dt,
               const glm::vec3 &gravity);
void integrate(RigidBody &rb, Transform3D &t, float dt,
               const glm::vec3 &gravity);
