#pragma once
#include "shader.h"
#include "uniform.h"

struct ShaderEntry {
    std::string vsPath, fsPath;                           // 다시 만들 때 필요
    Shader shader;                                        // GL 리소스
    std::unordered_map<std::string, UniformValue> params; // setFloat 저장소
};

class ShaderLib {
  public:
    bool add(const std::string &name, const std::string &vs,
             const std::string &fs);
    ShaderEntry *get(const std::string &name); // 없으면 nullptr
    void recreateAll();                        // INIT_WINDOW, F5
    void destroyAll();                         // TERM_WINDOW
  private:
    std::unordered_map<std::string, ShaderEntry> entries_;
};
