#version 330

// Input attributes
in vec3 in_position;

// Application data (no projection since it's UI element)
uniform mat3 transform;

void main()
{
  vec3 pos = vec3(in_position.xy, 1.0);
  // need to fix following:
  // vec3 pos = transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
