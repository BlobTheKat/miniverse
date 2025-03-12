layout(location=0) in vec4 xy_dxdy;
layout(location=1) in float radius;
layout(location=2) in highp uvec2 style;

uniform vec2 i_wh, xy_off;
uniform float t, dt;
out vec2 pos;
flat out lowp vec3 col1, col2, col3, col4, col5;
flat out highp uvec2 s;
flat out highp vec4 values;

void variant(out vec3 col2, int a){
	if(a < 24){
		int a1 = a>>3;
		vec3 c = vec3(col1.rbg[a1],col1.grb[a1],col1.bgr[a1]);
		float a = float(a&7)*.125;
		// 0.0 => rgb
		// 0.5 => 0.75rgb + 0.75gbr
		// 1.0 => gbr
		col2 = mix(c,c.brg,a);//c-c*a*a + c.brg*(2.-a)*a;
	}else if(a < 28) col2 = (col1-.5)*vec4(-.3,.3,-1,1.5)[a-24]+.5;
	else if(a < 30) col2 = col1*(float(31-a)*.25);
	else col2 = 1.-(1.-col1)*(float(33-a)*.25);
}

void micro_variant(out vec3 col2, vec3 c, int a){
	if(a < 7){
		if(a < 4) col2 = (c-.5)*vec4(1.,.8,1.2,1.4)[a]+.5;
		else col2 = 1.-(1.-c)*vec3(.85,.7,.5)[a-4];
	}else if(a < 10) col2 = c*vec3(.85,.7,.5)[a-7];
	else{
		float f = float(1<<a);
		col2 = a < 13 ? mix(c, c.brg, f*.00008138021) : mix(c, c.gbr, f*.000010172526);
	}
}

void main(){
	pos = vec2(gl_VertexID<<1&2, gl_VertexID&2)-1.;
	gl_Position = vec4((xy_dxdy.xy + xy_dxdy.zw*dt - xy_off + pos*radius)*i_wh,0,1);
	float y = float((4u|(style.x>>24&3u))<<(style.x>>26&3u))*.015;
	values = vec4(
		float(8>>(style.x>>30&3u))*vec4(.125,.1,.0833333333,.0714285714)[style.x>>28&3u]*y, y,
		float(style.y>>9&63u)*t*.04*y, float((4u|style.y&3u)<<(style.y>>2&3u))
	);
	pos = pos*.21650635 + pos.yx*vec2(.125,-.125); pos *= values.w;
	values.w = 4./(values.w-4.);
	s = style;
	col1 = vec3(style.x&31u, style.x>>5&31u, style.x>>10&31u)*.032258;
	variant(col2, int(style.x>>15&31u));
	micro_variant(col3, col1, int(style.y>>24&15u));
	micro_variant(col4, col2, int(style.y>>28&15u));
	variant(col5, int(style.y>>15&31u));
}