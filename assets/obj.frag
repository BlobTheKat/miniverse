#version 300 es
precision mediump float;
out vec4 color;
in vec2 pos;
in lowp vec4 col;

void main(){
	float a = (1.-length(pos));
	a = .5+a/fwidth(a);
	color = a*col;
}