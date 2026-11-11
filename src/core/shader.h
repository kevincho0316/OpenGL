#pragma once
#include "gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include <set>
#include <string>
#ifdef __ANDROID__
#include <android/asset_manager.h>
#endif

class Shader {
  public:
#ifdef __ANDROID__
    static AAssetManager *assets; // android_main에서 설정
#else
    static std::string assetDir; // 디스크 경로
#endif

    unsigned int ID = 0;
    int ok = 0;

    Shader() = default;
    Shader(const std::string &vsDir, const std::string &fsDir);

    std::string readFile(const std::string &path);
    std::string preprocess(const std::string &path,
                           std::set<std::string> &seen);

    void use() const;
    void destroy(); // 컨텍스트 손실 / 리로드 / 종료 시 호출
    void setBool(const std::string &n, bool v) const;
    void setInt(const std::string &n, int v) const;
    void setFloat(const std::string &n, float v) const;
    void setVec2(const std::string &n, glm::vec2 v) const;
    void setVec3(const std::string &n, glm::vec3 v) const;
    void setVec4(const std::string &n, glm::vec4 v) const;
    void setMat4(const std::string &n, const glm::mat4 &m) const;
    GLuint getUniLoc(const std::string &n) const;
};
