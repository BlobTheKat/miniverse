#include "rules.cpp"

namespace physics{

/*inline f32 fast_inverse(f32 x){
	// Best polynomial for log-accuracy: 0x7ef08eb3
	// Best polynomial for linear-accuracy: 0x7ef08dcb
	f32 f = bit_cast<f32>(0x7ef08eb3 - (bit_cast<u32>(x)));
	return f*(2-x*f);
}*/
// x*Q_sqrt(x)
inline f32 fast_sqrt(f32 x){
	f32 f = bit_cast<f32>(0x5f3759df - (bit_cast<u32>(x)>>1));
	return x*f*(1.5-x*f*f);
}

struct Node{
	static const u8 PINNED = 1, SELECTED = 2;
	Node* next;
	f64 x, y;
	f32 dx, dy;
	f32 radius;
	u8 flags; u8 bookmark_id;
	inline f32 rad_cap(f32 sq_dst){return max(sq_dst,radius*radius);}
	union{
		f32 mass;
		struct{ f32 props[]; };
	};
	Node* copy();
	static Node* create();
	Node(){}
	Node(f64 x, f64 y, f32 rad, f32 mass, f32 dx = 0, f32 dy = 0) : x(x), y(y), radius(rad), mass(mass), dx(dx), dy(dy){}
};

}