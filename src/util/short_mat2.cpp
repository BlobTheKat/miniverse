#include "defs.cpp"

template<typename F>
struct shmat2{
	F x, y, z, w;
	operator F*(){return &x;}
	inline void translate(F x, F y){ z += x*this->x; w += y*this->y; }
	inline void translateX(F x){ z += x*this->x; }
	inline void translateY(F y){ w += y*this->y; }
	inline void translate(vec<F,2>& to){ z += to.x*x; w += to.y*y; }
	inline void scale(F x, F y){ this->x *= x; this->y *= y; }
	inline void scaleX(F x){ this->x *= x; }
	inline void scaleY(F y){ this->y *= y; }
	inline void scale(vec<F,2>& by){ this->x *= by.x; this->y *= by.y; }
	inline void reset(vec<F,2> xy = vec<F,2>(0), vec<F,2> scale = vec<F,2>(1)){
		this->x = scale.x; this->y = scale.y;
		this->z = xy.x*scale.x; this->w = xy.y*scale.y;
	}
};