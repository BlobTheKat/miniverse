#version 300 es
precision mediump float;
uniform sampler2D atlas;
uniform vec2 distRange;
in vec2 uv; in vec4 color;
out vec4 col;
void main(){
	vec3 c = texture(atlas, uv).rgb;
	float sd = max(min(c.r, c.g), min(max(c.r, c.g), c.b))-.5;
	col = color*clamp(sd/dot(fwidth(uv),distRange)+.5,0.,1.);
}