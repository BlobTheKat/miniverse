#version 300 es

layout(location=0) in vec3 sprite;
layout(location=1) in lowp vec4 inColor;

uniform vec2 i_wh, xy_off;
out vec2 pos;
out lowp vec4 col;

void main(){
	pos = vec2(gl_VertexID<<1&2, gl_VertexID&2)-1.;
	gl_Position = vec4((sprite.xy - xy_off + pos*sprite.z)*i_wh,0,1);
	col = inColor;
}