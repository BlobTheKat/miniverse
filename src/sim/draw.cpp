#include "node.cpp"

namespace physics{

struct Sprite{
	vec2 pos; f32 radius;
};

thread_local vector<Sprite> drawBuf;
f64 cam_x, cam_y;

}