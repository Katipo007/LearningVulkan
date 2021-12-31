module;
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
export module Bridges.glm;

namespace glm
{
	export using glm::vec2;
	export using glm::vec3;
	export using glm::vec4;
	export using glm::ivec2;
	export using glm::ivec3;
	export using glm::ivec4;
	export using glm::mat4;
}
