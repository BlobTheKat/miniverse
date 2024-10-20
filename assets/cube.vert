#version 410
layout(location=0) in vec3 pos;
uniform mat4 m;
uniform vec3 cam;
out vec3 ipos, dpos;
uniform sampler3D vol;

void main(){
	ipos = (pos+1.)*.5*textureSize(vol, 0);
	dpos = pos-cam;
	gl_Position = m*vec4(pos,1);
}