#include "node.cpp"

namespace physics{

struct Sprite{
	vec2 pos; f32 radius; bvec4 color;
};

thread_local vector<Sprite> drawBuf;
f64 cam_x, cam_y;

}