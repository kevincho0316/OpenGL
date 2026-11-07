// Desktop
// shader.h
#pragma once
#include "gl.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include <set>
#include <string>

class Shader {
public:
  static std::string assetDir; // 안드로이드 AAssetManager 대신 디스크 경로

  unsigned int ID = 0;

  Shader() = default;
  Shader(const std::string &vsDir, const std::string &fsDir);

  std::string readFile(const std::string &path);
  std::string preprocess(const std::string &path, std::set<std::string> &seen);
  void use() const;
  void destroy(); // 셰이더 리로드 / 종료 시 호출
  void setBool(const std::string &n, bool v) const;
  void setInt(const std::string &n, int v) const;
  void setFloat(const std::string &n, float v) const;
  void setVec3(const std::string &n, glm::vec3 v) const;
  void setVec4(const std::string &n, glm::vec4) const;
  void setMat4(const std::string &n, const glm::mat4 &m) const;
  GLuint getUniLoc(const std::string &n) const;
};
