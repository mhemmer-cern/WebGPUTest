#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp> // all types inspired from GLSL

/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */
struct VertexAttributes {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
};
