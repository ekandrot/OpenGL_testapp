#version 330 core

in vec3 vertexPosition_modelspace;
in vec2 texture_coord;
uniform mat4 MVP;
out vec2 texCoord;

void main(){
	texCoord = texture_coord;
	gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
 }