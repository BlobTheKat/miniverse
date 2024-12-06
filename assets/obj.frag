#version 300 es
precision mediump float;
out vec4 color;
in vec2 pos;
void main(){
	float a = (1.-length(pos));
	a = .5+a/fwidth(a);
	color = vec4(a);
}