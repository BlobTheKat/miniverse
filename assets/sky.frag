#version 410
in vec3 vpos;
out vec4 color;
uniform float t;

uint uhash(uvec2 a){ 
	uint x = ((a.x * 1597334673U) ^ (a.y * 3812015801U));
	x = x * 0x7feb352du;
	x = x ^ (x >> 15u);
	x = x * 0x846ca68bu;
	return x;
}
#define lerp(a,b,c) mix(a,b,c*c*(3.f-2.f*c))
vec2 valnoise(vec2 uv){
	vec2 fa = floor(uv);
	uv -= fa;
	uvec2 i = uvec2(ivec2(fa));
	uint x = uhash(i);
	float a = float(x&0xffffu)/65535., b = float(x>>16)/65535.;
	i.x++; x = uhash(i);
	a = lerp(a, float(x&0xffffu)/65535., uv.x); b = lerp(b, float(x>>16)/65535., uv.x);
	i.y++; x = uhash(i);
	float a2 = float(x&0xffffu)/65535., b2 = float(x>>16)/65535.;
	i.x--; x = uhash(i);
	a2 = lerp(float(x&0xffffu)/65535., a2, uv.x); b2 = lerp(float(x>>16)/65535., b2, uv.x);
	return vec2(lerp(a, a2, uv.y), lerp(b, b2, uv.y));
}

void main(){
	gl_FragDepth = 1.;
	vec3 a = normalize(vpos);
	color = vec4(0, .5+a.y*.25, 1, 1);
	if(a.y > .1){
		vec2 uv = vpos.xz/vpos.y*10. + t*vec2(.15,.4);
		vec2 off = valnoise(uv);
   	vec2 x = valnoise(vec2(uv.x+uv.y, uv.x-uv.y)/2.8+off);
		color.rgb = lerp(color.rgb, vec3(.8+abs(x.x-.75)*.3+x.y*.2), clamp(0, x.x*min(2.,a.y*6.-.6)-.5, 1));
	}
}