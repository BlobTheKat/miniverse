#include "node.cpp"

namespace physics{

struct Sprite{
	vec2 pos, dxdy;
	f32 radius; bvec4 color;
};

struct ObjectVisual{
	u8 body_rad, rot_speed;
	u8 speed, noise2_bias;
	u8 noise1_w, noise1_h, noise2_w, noise2_h;
	bvec4 body_color_min, body_color_max;
	bvec4 body_color1_min, body_color1_max;
	bvec4 body_color2_min, body_color2_max;
	u8 corona_thickness, corona_blur;
	bvec4 corona_color;
	bvec4 atmosphere_color1, atmosphere_color2;
};

}