#include "node.cpp"

namespace physics{

struct Sprite{
	vec2 pos, dxdy;
	f32 radius; bvec4 color;
};

thread_local vector<Sprite> drawBuf;
thread_local f64 cam_x, cam_y, cam_hw, cam_hh;

}