#version 410
layout(location=0) in vec3 pos;
uniform mat4 m;
uniform mat3x4 o;
out vec3 vpos;
void main(){
	vpos = vec4(pos,1)*o;
	gl_Position = vec4(vpos,1)*m;
}