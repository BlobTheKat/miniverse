precision(highp float);
precision(highp int);

in vec2 pos;
out lowp vec4 color;

vec2 uhash(uvec2 a) { 
	uint x = ((a.x * 1597334673U) ^ (a.y * 3812015801U));
	x = x * 0x7feb352du;
	x = x ^ (x >> 15u);
	x = x * 0x846ca68bu;
	return vec2(x&0xffffu, (x>>17u) + (x&0xc0000000u))*.0000203450520833333;
}

float star(vec2 p){
	vec2 p1 = floor(p); p -= p1;
	uvec2 ixy = uvec2(ivec2(p1));
	vec2 h00 = uhash(ixy)-.5, h01 = uhash(ixy+uvec2(0,1))+vec2(-.5,.5);
	vec2 h10 = uhash(ixy+uvec2(1,0))+vec2(.5,-.5), h11 = uhash(ixy+1u)+.5;
	float a = min(.25, min(min(length(h00-p), length(h01-p)), min(length(h10-p), length(h11-p))));
	return clamp(.5+(.03-a)/fwidth(a), 0., 1.);
}

void main(){
	float a = star(pos*4.)*.5 + star(pos*16.)*.25;
	color = vec4(a, a, a, 1);
}