#pragma once

#include "core/Rigidbody.h"
#include "core/objectTransform.h"
#include "core/render.h"

#include <optional>
#include <variant>

struct object {
    std::variant<Transform2D, Transform3D> transform;
    std::optional<DrawCmd> renderer;
    std::optional<RigidBody> rigidbody;

    glm::mat4 model() const {
        return std::visit([](const auto &t) { return t.matrix(); }, transform);
    }
};
