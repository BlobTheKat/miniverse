out highp vec2 pos;
uniform highp vec2 wh;
void main(){
	pos = vec2(gl_VertexID<<1&2, gl_VertexID&2)-1.;
	gl_Position = vec4(pos,0.,1.);
	pos *= wh;
}