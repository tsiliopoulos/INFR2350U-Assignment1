#pragma once

#include "glm/glm.hpp"
#include <glm\gtx\color_space.hpp>

// Simple function to get a rgb color at the specified hue point
// t is meant to be between 0 and 1
glm::vec4 getColorFromHue(float t)
{
	return glm::vec4(glm::rgbColor( glm::vec3(glm::mix(0.0f, 360.0f, t), 1.0f, 0.5f)), 1.0f);
}