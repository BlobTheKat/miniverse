#include <stdio.h>
#define _LARGEFILE64_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>

#ifdef __INTELLISENSE__
	// vscode kinda loses it
   #pragma diag_suppress 2363
	#pragma diag_suppress 29
	typedef signed long long ssize_t;
#endif
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;
#if UINTPTR_MAX == UINT64_MAX
typedef double fsize;
#else
typedef float fsize;
#endif
typedef size_t usize;
typedef ssize_t isize;
typedef char byte;

struct vec2{
	float x,y;
	inline vec2(){this->x=this->y=0;}
	inline vec2(float x, float y){this->x=x;this->y=y;}
	inline vec2(float x){this->x=x;this->y=x;}
	#define OP(o) vec2 operator o (vec2 b){return vec2(this->x o b.x,this->y o b.y);}
	vec2 operator-(){return vec2(-this->x,-this->y);}
	OP(+) OP(-) OP(*) OP(/)
	OP(+=) OP(-=) OP(*=) OP(/=)
	#undef OP
};
struct vec3{
	float x,y,z;
	inline vec3(){this->x=this->y=this->z=0;}
	inline vec3(float x, float y, float z){this->x=x;this->y=y;this->z=z;}
	inline vec3(vec2 xy, float z){this->x=xy.x;this->y=xy.y;this->z=z;}
	inline vec3(float x, vec2 yz){this->x=x;this->y=yz.x;this->z=yz.y;}
	inline vec3(float x){this->x=x;this->y=x;this->z=x;}
	#define OP(o) vec3 operator o (vec3 b){return vec3(this->x o b.x,this->y o b.y,this->z o b.z);}
	vec3 operator-(){return vec3(-this->x,-this->y,-this->z);}
	OP(+) OP(-) OP(*) OP(/)
	OP(+=) OP(-=) OP(*=) OP(/=)
	#undef OP
};

struct Mat{
	float data[16];
	operator float*(){return data;}
	float& operator[](int i){return data[i];}
	void yaw(float y){
		float x,z, sy = sin(y), cy = cos(y);
		x = data[0]; z = data[2]; data[0] = cy*x+sy*z; data[2] = cy*z-sy*x;
		x = data[4]; z = data[6]; data[4] = cy*x+sy*z; data[6] = cy*z-sy*x;
		x = data[8]; z = data[10]; data[8] = cy*x+sy*z; data[10] = cy*z-sy*x;
		x = data[12]; z = data[14]; data[12] = cy*x+sy*z; data[14] = cy*z-sy*x;
	}
	void pitch(float x){
		float y,z, sx = sin(x), cx = cos(x);
		y = data[1]; z = data[2]; data[1] = cx*y+sx*z; data[2] = cx*z-sx*y;
		y = data[5]; z = data[6]; data[5] = cx*y+sx*z; data[6] = cx*z-sx*y;
		y = data[9]; z = data[10]; data[9] = cx*y+sx*z; data[10] = cx*z-sx*y;
		y = data[13]; z = data[14]; data[13] = cx*y+sx*z; data[14] = cx*z-sx*y;
	}
	void roll(float z){
		float x,y, sz = sin(z), cz = cos(z);
		y = data[1]; x = data[0]; data[1] = cz*y+sz*x; data[0] = cz*x-sz*y;
		y = data[5]; x = data[4]; data[5] = cz*y+sz*x; data[4] = cz*x-sz*y;
		y = data[9]; x = data[8]; data[9] = cz*y+sz*x; data[8] = cz*x-sz*y;
		y = data[13]; x = data[12]; data[13] = cz*y+sz*x; data[12] = cz*x-sz*y;
	}
	void translate(vec3 by){
		data[3] += (data[0]*by.x+data[1]*by.y+data[2]*by.z);
		data[7] += (data[4]*by.x+data[5]*by.y+data[6]*by.z);
		data[11] += (data[8]*by.x+data[9]*by.y+data[10]*by.z);
		data[15] += (data[12]*by.x+data[13]*by.y+data[14]*by.z);
	}
};

void cameraMatrix(Mat& m, float fovDeg, float ratio, float near, float far){
	// FOV degrees to half-FOV radians
	float S = 1/tan(fovDeg*.00872664625f);
	m[5] = S; m[0] = S/ratio;
	m[1] = m[2] = m[3] = m[4] = m[6] = m[7] = m[8] = m[9] = m[12] = m[13] = m[15] = 0;
	S = far/(near-far);
	m[10] = S;
	m[11] = S*near;
	m[14] = -1;
}
void idMatrix(Mat& m){
	m[0] = m[5] = m[10] = m[15] = 1;
	m[1] = m[2] = m[3] = m[4] = 0;
	m[6] = m[7] = m[8] = m[9] = 0;
	m[11] = m[12] = m[13] = m[14] = 0;
}