#version 330 core

// Positions/Coordinates
layout(location = 0) in uint aCamIndex;
// Positions/Coordinates
layout(location = 1) in vec3 aPos;
// Colors
layout(location = 2) in vec3 aColor;

// Outputs the color for the Fragment Shader
out vec3 v_Color;

// Controls the scale of the vertices
uniform float u_Scale;

// Inputs the matrices needed for 3D viewing with perspective for up to 3 cameras
uniform mat4 u_MVP0;
uniform mat4 u_MVP1;
uniform mat4 u_MVP2;

void main()
{
	// Outputs the positions/coordinates of all vertices
	if (aCamIndex == 0u) {
		gl_Position = u_MVP0 * vec4(u_Scale * aPos, 1.0);
	}
	else if (aCamIndex == 1u) {
		gl_Position = u_MVP1 * vec4(u_Scale * aPos, 1.0);
	}
	else if (aCamIndex == 2u) {
		gl_Position = u_MVP2 * vec4(u_Scale * aPos, 1.0);
	}
	else  {
		gl_Position = u_MVP0 * vec4(u_Scale * aPos, 1.0);
	}

	// Assigns the colors from the Vertex Data to "color"
	v_Color = aColor;
}