#shader vertex
#version 330 core

// Positions/Coordinates
layout(location = 0) in vec3 aPos;
// Colors
layout(location = 1) in vec3 aColor;

// Outputs the color for the Fragment Shader
out vec3 v_Color;

// Controls the scale of the vertices
uniform float u_Scale;

// Controls the scale of the vertices
uniform mat4 u_Intrinsics;

// Inputs the matrices needed for 3D viewing with perspective
uniform mat4 u_MVP;

void main()
{
	// Outputs the positions/coordinates of all vertices
	gl_Position = u_MVP * vec4(u_Scale * aPos, 1.0);
	// Assigns the colors from the Vertex Data to "color"
	v_Color = aColor;
}

#shader fragment
#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Inputs the color from the Vertex Shader
in vec3 v_Color;

void main()
{
	FragColor = vec4(v_Color, 1);
}