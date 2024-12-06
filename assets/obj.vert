#version 300 es
layout(location=0) in vec3 sprite;
uniform vec2 i_wh;
out vec2 pos;

void main(){
	pos = vec2(gl_VertexID<<1&2, gl_VertexID&2)-1.;
	gl_Position = vec4((sprite.xy + pos*sprite.z)*i_wh,0,1);
}