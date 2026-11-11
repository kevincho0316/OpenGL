// src/core/uniform.h
#pragma once
#include "core/shader.h"
#include <glm/glm.hpp>
#include <string>
#include <type_traits>
#include <variant>

using UniformValue =
    std::variant<bool, int, float, glm::vec2, glm::vec3, glm::vec4, glm::mat4>;

struct ShaderUniform {
    std::string name;
    UniformValue value;
};

// 셰이더가 use() 된 상태에서 호출할 것
inline void apply(const Shader &s, const std::string &name,
                  const UniformValue &value) {
    std::visit(
        [&](const auto &v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)
                s.setBool(name, v);
            else if constexpr (std::is_same_v<T, int>)
                s.setInt(name, v);
            else if constexpr (std::is_same_v<T, float>)
                s.setFloat(name, v);
            else if constexpr (std::is_same_v<T, glm::vec2>)
                s.setVec2(name, v);
            else if constexpr (std::is_same_v<T, glm::vec3>)
                s.setVec3(name, v);
            else if constexpr (std::is_same_v<T, glm::vec4>)
                s.setVec4(name, v);
            else if constexpr (std::is_same_v<T, glm::mat4>)
                s.setMat4(name, v);
        },
        value);
}

inline void apply(const Shader &s, const ShaderUniform &u) {
    apply(s, u.name, u.value);
}
