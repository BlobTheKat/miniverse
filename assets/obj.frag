#version 300 es
precision mediump float;
out vec4 color;
in vec2 pos;
in lowp vec4 col;
uniform sampler2D noise;
uniform float t;

struct Visual{
	float body_rad, rot_speed;
	float speed, noise2_bias, noise1_factor, noise2_factor;
	vec2 noise1_wh, noise2_wh;
	vec4 body_color_min, body_color_max;
	vec4 body_color1_min, body_color1_max;
	vec4 body_color2_min, body_color2_max;
	float corona_thickness;
	vec4 corona_color;
	vec4 atmosphere_color1, atmosphere_color2;
};
const Visual visual = Visual(0.5, 1., 1., -.1, 5000., 4., vec2(1), vec2(1), vec4(0,.2,.9,1), vec4(0,.2,.9,1), vec4(.1,.7,0,1), vec4(.1,.7,0,1), vec4(1), vec4(1), .05, vec4(0,0,0,1), vec4(0,.5,1,1),vec4(0));

float alpha(float a){return clamp(a/fwidth(a)+.5,0.,1.);}
vec4 compose(vec4 bg, vec4 fg){return bg*(1.-fg.a)+fg;}
void main(){
	float a = length(pos) - visual.body_rad, b = a/visual.body_rad;
	vec2 uv2 = pos/(sqrt(-a)+.5)*.25;
	float noise1 = clamp((texture(noise, (uv2+.5)*visual.noise1_wh*.5).x-.5)*visual.noise1_factor+.5,0.,1.);
	float noise2 = clamp((texture(noise, (uv2+visual.speed*t*vec2(.05,.02))*visual.noise2_wh-.5).x-.5+visual.noise2_bias)*visual.noise2_factor+.5,0.,1.);
	
	color = mix(visual.body_color_min, visual.body_color_max, .25);
	color = mix(color, mix(visual.body_color1_min, visual.body_color1_max, .5), noise1);
	color = compose(color, mix(visual.body_color2_min, visual.body_color2_max, .75)*noise2);
	color.xyz *= (b-1.)*-.5;
	color = compose(color, visual.corona_color*alpha(1.+b/visual.corona_thickness));
	if(a > 0.){
		color = mix(color, visual.atmosphere_color1, alpha(a));
		color = mix(color, visual.atmosphere_color2, a/(1.-visual.body_rad));
	}
	//float a = (1.-length(pos));
	//a = .5+a/fwidth(a);
	//color = a*col;
}

/*
struct Visual{
	float body_rad, rot_speed;
	float speed, noise2_bias, noise1_factor, noise2_factor;
	vec2 noise1_wh, noise2_wh;
	vec4 body_color_min, body_color_max;
	vec4 body_color1_min, body_color1_max;
	vec4 body_color2_min, body_color2_max;
	float corona_thickness;
	vec4 corona_color;
	vec4 atmosphere_color1, atmosphere_color2;
};
Visual visual = Visual(0.5, 1., 1., -.1, 5000., 4., vec2(1), vec2(1), vec4(0,0,1,1), vec4(0,0,1,1), vec4(0,1,0,1), vec4(0,1,0,1), vec4(1), vec4(1), .05, vec4(0,0,0,1), vec4(0,.5,1,1),vec4(0));

float s = 0.;
float seed(){s=mod(s+.6503,1.);return s;}
float alpha(float a){return clamp(a/fwidth(a)+.5,0.,1.);}
vec4 compose(vec4 bg, vec4 fg){return bg*(1.-fg.a)+fg;}
vec4 main(vec2 uv){
	uv = uv*2.-1.;
	float a = length(uv) - visual.body_rad, b = a/visual.body_rad;
	vec2 uv2 = uv/(sqrt(-a)+.5)*.25;
	float noise1 = clamp((getColor(images.noise, (uv2+.5)*visual.noise1_wh*.5).x-.5)*visual.noise1_factor+.5,0.,1.);
	float noise2 = clamp((getColor(images.noise, (uv2+visual.speed*vec2(.05,.02)*t)*visual.noise2_wh-.5).x-.5+visual.noise2_bias)*visual.noise2_factor+.5,0.,1.);
	
	vec4 col = mix(visual.body_color_min, visual.body_color_max, seed());
	col = mix(col, mix(visual.body_color1_min, visual.body_color1_max, seed()), noise1);
	col = compose(col, mix(visual.body_color2_min, visual.body_color2_max, seed())*noise2);
	col.xyz *= (b-1.)*-.5;
	col = compose(col, visual.corona_color*alpha(1.+b/visual.corona_thickness));
	if(a > 0.){
		col = mix(col, visual.atmosphere_color1, alpha(a));
		col = mix(col, visual.atmosphere_color2, a/(1.-visual.body_rad));
	}
	return col;
}
*/