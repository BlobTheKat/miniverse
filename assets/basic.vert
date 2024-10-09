#version 410
layout(location=0) in vec3 pos;
uniform mat4 m;
uniform mat4x3 o;
out vec3 vpos;
void main(){
	vpos = o*vec4(pos,1);
	gl_Position = m*vec4(vpos,1);
}