#version 410
layout(location=0) in vec3 pos;
uniform mat4 m;
out vec3 vpos;
void main(){
	gl_Position = m*vec4(vpos = pos,1);
}