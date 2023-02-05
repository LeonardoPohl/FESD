#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Inputs the color from the Vertex Shader
in vec3 v_Color;

void main()
{
	FragColor = vec4(v_Color, 1);
}