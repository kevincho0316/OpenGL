// Shared (desktop + android)
// camera.cpp
#include "camera.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

glm::vec3 Camera::front() const {
  float y = glm::radians(yaw), p = glm::radians(pitch);
  return glm::normalize(
      glm::vec3(std::cos(y) * std::cos(p), std::sin(p), std::sin(y) * std::cos(p)));
}

glm::vec3 Camera::right() const {
  return glm::normalize(glm::cross(front(), worldUp));
}

glm::vec3 Camera::up() const { return glm::normalize(glm::cross(right(), front())); }

void Camera::setRotation(float yawDeg, float pitchDeg) {
  yaw = yawDeg;
  pitch = std::clamp(pitchDeg, -89.0f, 89.0f); // 90 이면 front 와 up 이 평행 → lookAt 깨짐
}

void Camera::lookAt(const glm::vec3 &target) {
  glm::vec3 d = target - position;
  if (glm::length(d) < 1e-6f)
    return;
  d = glm::normalize(d);
  setRotation(glm::degrees(std::atan2(d.z, d.x)), glm::degrees(std::asin(d.y)));
}

void Camera::move(const glm::vec3 &local) {
  position += right() * local.x + up() * local.y + front() * local.z;
}

glm::mat4 Camera::view() const {
  return glm::lookAt(position, position + front(), up());
}

glm::mat4 Camera::projection(float aspect) const {
  if (perspective)
    return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
  float h = orthoSize, w = orthoSize * aspect;
  return glm::ortho(-w, w, -h, h, zNear, zFar);
}
