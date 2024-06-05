#version 410
out vec4 color;
in vec2 pos;
void main(){
	color = vec4(pos, 0, 1);
}