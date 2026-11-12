#include "../gl.h"
#include "glm/detail/qualifier.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/fwd.hpp"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

class RigidBody {
  private:
    glm::vec3 vel_;
    glm::vec3 accel_;

  public:
    glm::vec3 get_vel() { return vel_; }
    glm::vec3 set_vel(glm::vec3 v) { vel_ = v; }

    glm::vec3 get_accel() { return accel_; }
    glm::vec3 set_accel(glm::vec3 a) { accel_ = a; }
};
