#version 300 es
precision mediump float;
out vec4 color;
in vec2 pos;
flat in lowp vec3 col1, col2, col3, col4, col5;
flat in mediump uvec2 s;
flat in highp vec4 values;
uniform sampler2D noise;

float alpha(float a){return clamp(a/fwidth(a),0.,1.);}
vec4 compose(vec4 bg, vec4 fg){return bg*(1.-fg.a)+fg;}

void main(){
	float a = length(pos);
	vec2 uv2 = pos/((sqrt(max(0.,1.-a))+.5)*max(1.,a)); a -= 1.;
	vec2 uv1 = uv2*values.xy; uv1.x += values.z;
	float n = texture(noise, uv1).x*2.-1.;
	color = vec4(mix(col1, col2, clamp(n*vec4(16000,3.75,1.5,.75)[s.x>>20&3u]+.5,0.,1.)),1);
	int x = int(n>0.);
	color.rgb = mix(color.rgb, mat2x3(col3, col4)[x], vec2(abs(n),min(n*n*3.,1.))[int(s>>(x+7)&1u)]*1.5);
	//float noise2 = clamp((texture(noise, (uv2+SPEED*t*vec2(.05,.02))-.5).x-.6)+.5,0.,1.);
	//color = compose(color, mix(visual.body_color2_min, visual.body_color2_max, .75)*noise2);
	//color = compose(color, visual.corona_color*alpha(1.+a/visual.corona_thickness));
	float g = color.r+color.g+color.b;
	vec3 glow = color.rgb*vec4(0.,.02,.04,.08)[s.x>>22&3u];
	float z = clamp(a*vec4(16000.,2.,1.,.5)[s.y>>5&3u]+.7,0.,1.); z *= z;
	color.rgb = color.rgb*(1.-z)+float(s.y>>4&1u)*z;
	float a1 = alpha(a);
	if(a > 0.){
		color.rgb = mix(color.rgb, col5, a1);
		a = 1.-a*values.w; g = max(0., g*a*1.3);
		color.rgb += g*g*g*glow;
		color *= a;
	}else color.rgb += g*g*g*glow;
}