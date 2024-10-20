#version 410
out vec4 color;
in vec3 vpos;
uniform vec4 col;
void main(){
	vec2 a = abs(normalize(cross(dFdx(vpos), dFdy(vpos))).xz);
	float normalTint = 1.-.1*a.y-.2*a.x;
	color.rgb = col.rgb*normalTint;
	color.a = col.a;
}