#version 330 core

// Positions/Coordinates
layout(location = 0) in vec3 aPos;
// Colors
layout(location = 1) in vec3 aColor;
// Positions/Coordinates
layout(location = 2) in int aCamIndex;

// Outputs the color for the Fragment Shader
out vec3 v_Color;

// Inputs the matrices needed for 3D viewing with perspective for up to 4 cameras
uniform mat4 u_Model;
uniform mat4 u_VP;
uniform bool u_AlignmentMode;

void main()
{
	// Outputs the positions/coordinates of all vertices 
	if (aCamIndex == 0){
		gl_Position = u_VP * u_Model * vec4(aPos, 1.0);
	}else{
		gl_Position = u_VP * vec4(aPos, 1.0);
	}
	
	// Assigns the colors from the Vertex Data to "color"
	if (!u_AlignmentMode){
		v_Color = aColor;
	}else{
		if (aCamIndex == 0){
			v_Color = vec3(0, 0, 0);
		}else{
			v_Color = vec3(1, 1, 1);
		}
	}
}