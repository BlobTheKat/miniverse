#version 300 es
layout(location=0) in vec4 data;
layout(location=1) in vec4 col;
out vec2 uv; out vec4 color;
void main(){
	gl_Position = vec4(data.xy,0.,1.);
	uv = data.zw; color = col;
}