// Shared (desktop + android)
// camera.h — GL 의존성 없음, glm 만 사용
#pragma once
#include <glm/glm.hpp>

class Camera {
public:
  glm::vec3 position{0.0f, 0.0f, 1.0f};
  float yaw = -90.0f; // 도(degree), -90 = -Z 방향을 바라봄
  float pitch = 0.0f; // 도, ±89 로 제한
  glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

  bool perspective = false; // false = ortho (기본값: 기존 2D 화면과 동일)
  float fov = 45.0f;        // perspective 시야각 (도)
  float orthoSize = 1.0f;   // ortho 화면 세로 절반 크기
  float zNear = 0.1f;
  float zFar = 100.0f;

  glm::vec3 front() const;
  glm::vec3 right() const;
  glm::vec3 up() const;

  void setRotation(float yawDeg, float pitchDeg);
  void lookAt(const glm::vec3 &target); // target 쪽으로 yaw/pitch 계산
  void move(const glm::vec3 &local);    // x = 오른쪽, y = 위, z = 앞

  glm::mat4 view() const;
  glm::mat4 projection(float aspect) const;
};
