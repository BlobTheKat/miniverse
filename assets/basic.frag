#version 410
in vec3 vpos;
out vec4 color;
uniform vec4 col;
void main(){
	vec3 a = dFdx(vpos), b = dFdy(vpos);
	a = abs(normalize(cross(a, b)));
	float normalTint = 1.-.1*a.z-.2*a.x;
	color.rgb = col.rgb*normalTint;
	color.a = col.a;
}