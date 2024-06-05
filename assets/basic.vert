#version 410
out vec2 pos;
void main(){
	pos = vec2(gl_VertexID&1, gl_VertexID>>1);
	gl_Position = vec4(pos-.5, 0, 1);
}