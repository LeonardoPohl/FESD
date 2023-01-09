#version 330 core

// Positions/Coordinates
layout(location = 0) in uint aCamIndex;
// Positions/Coordinates
layout(location = 1) in vec3 aPos;
// Colors
layout(location = 2) in vec3 aColor;

// Outputs the color for the Fragment Shader
out vec3 v_Color;

// Inputs the matrices needed for 3D viewing with perspective for up to 4 cameras
uniform mat4 u_MVP[4];

void main()
{
	// Outputs the positions/coordinates of all vertices
	if (aCamIndex < 4u) {
		gl_Position = u_MVP[aCamIndex] * vec4(aPos, 1.0);
	}
	else{
		gl_Position = u_MVP[0] * vec4(aPos, 1.0);
	}

	// Assigns the colors from the Vertex Data to "color"
	v_Color = aColor;
}