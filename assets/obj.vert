#version 300 es

layout(location=0) in vec4 xy_dxdy;
layout(location=1) in float radius;
layout(location=2) in lowp vec4 inColor;

uniform vec2 i_wh, xy_off;
uniform float dt;
out vec2 pos;
out lowp vec4 col;

void main(){
	pos = vec2(gl_VertexID<<1&2, gl_VertexID&2)-1.;
	gl_Position = vec4((xy_dxdy.xy + xy_dxdy.zw*dt - xy_off + pos*radius)*i_wh,0,1);
	col = inColor;
}