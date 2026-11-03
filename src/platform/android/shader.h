// Android
// shader.h
#pragma once
#include "gl.h"
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
#include <set>
#include <string>

class Shader {
  std::set<std::string> seen;

public:
  static AAssetManager *assets;

  unsigned int ID = 0;

  Shader() = default;
  Shader(const std::string &vsDir, const std::string &fsDir);

  std::string readFile(const std::string &path);
  std::string preprocess(const std::string &path, std::set<std::string> &seen);
  void use() const;
  void destroy(); // 컨텍스트 손실 시 호출
  void setBool(const std::string &n, bool v) const;
  void setInt(const std::string &n, int v) const;
  void setFloat(const std::string &n, float v) const;
  void setVec3(const std::string &n, float x, float y, float z) const;
  void setVec4(const std::string &n, float x, float y, float z, float w) const;
  GLuint getUniLoc(const std::string &n) const;
};
