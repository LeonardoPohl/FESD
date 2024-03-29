#version 330 core

// Position function
layout(location = 0) in vec3 aPosFun;
// Depth
layout(location = 1) in float aDepth;
// Colors
layout(location = 2) in vec3 aColor;
// Positions/Coordinates
layout(location = 3) in int aCamIndex;

// Outputs the color for the Fragment Shader
out vec3 v_Color;

// Inputs the matrices needed for 3D viewing with perspective for up to 4 cameras
uniform mat4 u_Model;
uniform mat4 u_VP;
uniform bool u_AlignmentMode;

void main()
{
	vec3 pos = vec3(aPosFun[0] * aDepth, aPosFun[1] * aDepth, aDepth);
	// Outputs the positions/coordinates of all vertices 
	if (aCamIndex == 0){
		gl_Position = u_VP * u_Model * vec4(pos, 1.0);
	}else{
		gl_Position = u_VP * vec4(pos, 1.0);
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