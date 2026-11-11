#include "objectTransform.h"
#include "render.h"
#include <optional>
#include <variant>

struct object {
    std::variant<Transform2D, Transform3D> transform;
    std::optional<Renderer> renderer;
};
