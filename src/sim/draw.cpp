#include "node.cpp"

using f4 = unsigned char;
constexpr f4 operator""_f4(unsigned long long xi){
	if((long long)xi<1) return 0;
	if(xi>=16) return 15;
	int a; float x = frexp((float)xi, &a);
	return int(x*8)-4 | (a-1)<<2;
}

constexpr f4 operator""_f4(long double x){
	if(x<1.) return 0;
	if(x>=16.) return 15;
	int a; x = frexpl(x, &a);
	return int(x*8)-4 | (a-1)<<2;
}

namespace physics{

struct Sprite{
	vec2 pos, dxdy;
	f32 radius; uvec2 style;
};

struct style{
	enum ColorVariant{
		SAME = 0, HUE_15DEG = 1, HUE_30DEG = 2, HUE_45DEG = 3,
		HUE_60DEG = 4, HUE_75DEG = 5, HUE_90DEG = 6, HUE_105DEG = 7,
		HUE_120DEG = 8, HUE_135DEG = 9, HUE_150DEG = 10, HUE_165DEG = 11,
		HUE_180DEG = 12, HUE_195DEG = 13, HUE_210DEG = 14, HUE_225DEG = 15,
		HUE_240DEG = 16, HUE_255DEG = 17, HUE_270DEG = 18, HUE_285DEG = 19,
		HUE_300DEG = 20, HUE_315DEG = 21, HUE_330DEG = 22, HUE_345DEG = 23,
		INVERT_30 = 24, DESAT_30 = 25, INVERT = 26, SAT_50 = 27,
		DARKEN_25 = 28, DARKEN_50 = 29, BRIGHTEN_25 = 30, BRIGHTEN_50 = 31
	};

	vec3 col1 = vec3(1);
	ColorVariant col2 = INVERT;
	enum{ BLEND_0 = 0, BLEND_20 = 1, BLEND_50 = 2, BLEND_100 = 3 } blend = BLEND_100;
	enum{ GLOW_0 = 0, GLOW_WEAK = 1, GLOW_MEDIUM = 2, GLOW_STRONG = 3 } glow = GLOW_0;
	f32 speed = .25; f4 noise_freq = 1_f4, noise_stretch = 1_f4;
	ColorVariant atm_col = SAME; f4 atm_size = 1_f4;
	enum{
		GRADIENT_0 = 0,
		SMALL_BLACK = 2, SMALL_WHITE = 3,
		MEDIUM_BLACK = 4, MEDIUM_WHITE = 5,
		FULL_BLACK = 6, FULL_WHITE = 7
	} gradient = GRADIENT_0;

	enum struct MicroV{
		SAME = 0, DESAT_20 = 1, SAT_20 = 2, SAT_40 = 3,
		BRIGHTEN_15 = 4, BRIGHTEN_30 = 5, BRIGHTEN_50 = 6, DARKEN_15 = 7, DARKEN_30 = 8, DARKEN_50 = 9,
		HUE_10 = 10, HUE_20 = 11, HUE_40 = 12, HUE_350 = 13, HUE_340 = 14, HUE_320 = 15
	};
	enum MicroVShape{ NORMAL = 0, SHARP = 1 };

	MicroVShape col1_v_size = NORMAL; MicroV col1_v = MicroV::SAME;
	MicroVShape col2_v_size = NORMAL; MicroV col2_v = MicroV::SAME;

	constexpr operator uvec2(){
		u32 col = clamp(int(col1.x*32), 0, 31) | clamp(int(col1.y*32), 0, 31)<<5 | clamp(int(col1.z*32), 0, 31)<<10;
		return {col|col2<<15|blend<<20|glow<<22|noise_freq<<24|noise_stretch<<28, atm_size|gradient<<4|col1_v_size<<7|col2_v_size<<8|clamp(int(speed*32), 0, 63)<<9|atm_col<<15|int(col1_v)<<24|int(col2_v)<<28};
	}
};

}