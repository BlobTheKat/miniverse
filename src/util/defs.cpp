#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <type_traits>
#include <tuple>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <bit>
#include <atomic>
#include <thread>
#include <syncstream>
#include <numeric>
#include <unordered_set>

// Windows
#ifdef _WIN32
//namespace win32{
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
	#undef WIN32_LEAN_AND_MEAN
	#undef NOMINMAX
	// Top tier windows trolling
	#undef near
	#undef far
	#undef pascal
	#undef cdecl
//}
	typedef SSIZE_T ssize_t;
	typedef SIZE_T size_t;
	inline void* page_alloc(size_t c = 1){void* a=VirtualAlloc(0, c<<16, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);if(!a)abort();return a;}
	inline void page_free(void* ptr, size_t c = 1){VirtualFree(ptr, 0, MEM_RELEASE);}
#else
	#include <sys/types.h>
	#include <sys/mman.h>
	inline void* page_alloc(size_t c = 1){void* a=mmap(0, c<<16, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);if(uintptr_t(a)==-1)abort();return a;}
	inline void page_free(void* ptr, size_t c = 1){munmap(ptr, c<<16);}
#endif

using namespace std;
using enum memory_order;

#ifndef __INTELLISENSE__
#define asset(a) ([]{extern char _binary_assets_ ## a ## _start __asm__("_binary_assets_" #a "_start"), _binary_assets_ ## a ## _end __asm__("_binary_assets_" #a "_end");return buffer{&_binary_assets_ ## a ## _start, (size_t)(&_binary_assets_ ## a ## _end-&_binary_assets_ ## a ## _start)};}())
#define asset_start(a) ([]{extern char _binary_assets_ ## a ## _start __asm__("_binary_assets_" #a "_start"), _binary_assets_ ## a ## _end __asm__("_binary_assets_" #a "_end");return &_binary_assets_ ## a ## _start;}())
#define asset_end(a) ([]{extern char _binary_assets_ ## a ## _end __asm__("_binary_assets_" #a "_end");return &_binary_assets_ ## a ## _end;}())
#define asset_size(a) ([]{extern char _binary_assets_ ## a ## _start __asm__("_binary_assets_" #a "_start"), _binary_assets_ ## a ## _end __asm__("_binary_assets_" #a "_end");return (size_t)(&_binary_assets_ ## a ## _end-&_binary_assets_ ## a ## _start);}())
#else
#define asset(a) buffer{0,0}
#define asset_start(a) 0
#define asset_end(a) 0
#define asset_size(a) 0
#endif
struct buffer{
	char* data; size_t size;
	operator void*(){return data;}
	operator size_t(){return size;}
};

template<size_t align = 1>
inline constexpr size_t adv_aligned(size_t x){ return (x + (align - 1)) / align * align; }
template<> inline constexpr size_t adv_aligned<1>(size_t x){ return x; }
template<> inline constexpr size_t adv_aligned<2>(size_t x){ return (x+1)&-2; }
template<> inline constexpr size_t adv_aligned<4>(size_t x){ return (x+3)&-4; }
template<> inline constexpr size_t adv_aligned<8>(size_t x){ return (x+7)&-8; }
template<> inline constexpr size_t adv_aligned<16>(size_t x){ return (x+15)&-16; }

template<size_t a, size_t b, size_t... rest>
inline constexpr size_t adv_aligned(size_t x){ return adv_aligned<lcm(a, b), rest...>(x); }

typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;

#ifdef _Float32
#define _Float16 f16;
typedef _Float32 f32;
typedef _Float64 f64;
#ifdef _Float128
#define _Float128 f128;
#endif
#else
typedef float f32;
typedef double f64;
#endif
#if UINTPTR_MAX == UINT64_MAX
typedef f64 fptr;
#else
typedef f32 fptr;
#endif
#if SIZE_MAX == UINT64_MAX
typedef f64 fsize;
#else
typedef f32 fsize;
#endif
typedef size_t usize;
typedef ssize_t isize;
typedef uintptr_t uptr;
typedef intptr_t iptr;
typedef char byte;

#define ci constexpr inline
template<typename F, size_t L> requires (L > 0) struct vec;
template<typename F, size_t L>
struct _vec_c_types{
	union{
		F data[L];
		vec<F, 2> xy;
		vec<F, 3> xyz;
		vec<F, 4> xyzw;
		struct{ F x; union{
			vec<F,2> yz; vec<F,3> yzw; vec<F,L-1> __rest;
			struct{ F y; union{
				vec<F,2> zw; struct{ F z; F w; };
			}; };
		}; };
	};
	template<typename... Ts>
	ci _vec_c_types(F x, Ts... a) : x(x), __rest(a...){}
};
template<typename F> struct _vec_c_types<F, 4>{
	union{
		F data[4];
		vec<F, 2> xy;
		vec<F, 3> xyz;
		struct{ F x; union{
			vec<F,2> yz; vec<F,3> yzw, __rest;
			struct{ F y; union{
				vec<F,2> zw; struct{ F z; F w; };
			}; };
		}; };
	};
	template<typename... Ts>
	ci _vec_c_types(F x, Ts... a) : x(x), yzw(a...){}
};
template<typename F> struct _vec_c_types<F, 3>{
	union{
		F data[3];
		vec<F, 2> xy;
		struct{ F x; union{
			vec<F,2> yz, __rest;
			struct{ F y; F z; };
		}; };
	};
	template<typename... Ts>
	ci _vec_c_types(F x, Ts... a) : x(x), yz(a...){}
};
template<typename F> struct _vec_c_types<F, 2>{
	union{
		F data[2];
		struct{ F x; union{ F y; F __rest; }; };
	};
	template<typename... Ts>
	ci _vec_c_types(F x, Ts... a) : x(x), y(a...){}
};

double operator""_deg(long double d){return d*0.017453292519943295;}

#define PI 3.141592653589793
#define PI2 6.283185307179586
template<typename F, typename... X>
F hypot(X... x){ return sqrt((0 + ... + (x * x))); }

template<typename F, size_t R, size_t C> struct _mat;

template<typename F, size_t L> requires (L > 0)
struct vec: _vec_c_types<F,L>{
	static const size_t size = L;
	using component_type = F;
	template<typename... Ts>
	ci vec(F a, Ts... b) : _vec_c_types<F,L>(a, b...){}
	template<typename X, typename... Ts> requires (X::size > 1)
	ci vec(X& a, Ts... b) : _vec_c_types<F,L>(a.x, a.__rest, b...){}
	ci vec(F a) : _vec_c_types<F,L>(a, a){}
	ci vec() : _vec_c_types<F,L>(0){}
	ci F& operator[](size_t i){return this->data[i];}
	#define OP(o, o2, d) ci vec<F,L> operator o o2(d) const{return {o this->x o2, o this->__rest o2};}
	OP(+,,) OP(-,,) OP(++,,) OP(--,,) OP(,++,int) OP(,--,int)
	#undef OP
	ci F sum() const{return this->x + this->__rest.sum();};
	ci F sqsum() const{return this->x*this->x + this->__rest.sqsum();};
	ci F length() const{return sqrt(sqsum());};
	
	ci bool operator!() const{return !this->x && !this->__rest;}
	explicit ci operator bool() const{return (bool)this->x || (bool)this->__rest;}

	template<typename F2>
	ci operator vec<F2,L>(){return {(F2)this->x,this->__rest};}

	//inline operator F*(){ return this->data; }
	#define OP(o,o2) \
		ci vec<F,L> operator o(vec<F,L> b) const{return {this->x o b.x, this->__rest o b.__rest};} \
		ci vec<F,L> operator o(F b) const{return {this->x o b, this->__rest o b};} \
		ci vec<F,L>& operator o2(vec<F,L> b){this->x o2 b.x, this->__rest o2 b.__rest;return *this;} \
		ci vec<F,L>& operator o2(F b){this->x o2 b, this->__rest o2 b;return *this;}
	OP(+, +=) OP(-, -=) OP(*, *=) OP(/, /=) OP(%, %=) OP(&, &=) OP(|, |=) OP(^, ^=) OP(<<, <<=) OP(>>, >>=)
	#undef OP
	template<size_t O> vec<F,O> operator*(const _mat<F,O,L>& m){
		vec<F,O> a;
		F* b = a, *i = m.data, *end = i + O*L;
		for(; i < end; i += L){
			F acc = 0;
			for(size_t j = 0; j < L; j++) acc += this->data[j] * i[j];
			*b++ = acc;
		}
		return a;
	}
	vec<F,L>& operator*=(const _mat<F,L,L>& m){
		vec<F,L> a = this;
		F* b = data, *i = m.data, *end = i + L*L;
		for(; i < end; i += L){
			F acc = 0;
			for(size_t j = 0; j < L; j++) acc += a[j] * i[j];
			*b++ = acc;
		}
		return *this;
	}
};

template<typename F>
struct vec<F,1>{
	static const size_t size = 1;
	using component_type = F;
	union{ F data[1]; F x; };
	inline vec(F a = 0) : x(a){}
	template<typename... Ts>
	inline vec(F a, Ts...) : x(a){}
	template<typename X, typename... Ts> requires (X::size)
	inline vec(X& a, Ts...) : x(a.x){}
	inline vec() : x(0){}
	inline F& operator[](size_t i){return this->x;}
	#define OP(o, o2, d) ci vec<F,1> operator o o2(d) const{return {o x o2};}
	OP(+,,) OP(-,,) OP(++,,) OP(--,,) OP(,++,int) OP(,--,int)
	#undef OP
	ci F sum() const{return x;};
	ci F sqsum() const{return x*x;};
	ci F length() const{return sqrt(x*x);};
	ci bool operator!() const{return !x;}
	ci operator bool() const{return (bool)x;}
	ci operator F(){ return x; }
	ci operator F*(){ return data; }
	#define OP(o,o2) \
		ci vec<F,1> operator o(vec<F,1> b) const{return {x o b->x};} \
		ci vec<F,1>& operator o2(vec<F,1> b){x o2 b->x;return *this;} \
		ci vec<F,1> operator o(F b) const{return {x o b};} \
		ci vec<F,1>& operator o2(F b){x o2 b;return *this;}
	OP(+, +=) OP(-, -=) OP(*, *=) OP(/, /=) OP(%, %=) OP(&, &=) OP(|, |=) OP(^, ^=) OP(<<, <<=) OP(>>, >>=)
	#undef OP
	template<size_t O> vec<F,O> operator*(const _mat<F,O,1>& m){
		vec<F,O> a;
		F* b = a, *i = m.data, *end = i + O;
		for(; i < end; i++) *b++ = x * i[0];
		return a;
	}
	vec<F,1>& operator*=(const _mat<F,1,1>& m){
		x *= m.data[0];
		return *this;
	}
};

using vec2 = vec<f32,2>; using dvec2 = vec<f64,2>; using ivec2 = vec<i32,2>; using uvec2 = vec<u32,2>; using bvec2 = vec<u8,2>;
using vec3 = vec<f32,3>; using dvec3 = vec<f64,3>; using ivec3 = vec<i32,3>; using uvec3 = vec<u32,3>; using bvec3 = vec<u8,3>;
using vec4 = vec<f32,4>; using dvec4 = vec<f64,4>; using ivec4 = vec<i32,4>; using uvec4 = vec<u32,4>; using bvec4 = vec<u8,4>;
static_assert(sizeof(vec2) == 2*sizeof(float));


// Column major
// [ 0  4  8 12 ]
// [ 1  5  9 13 ]
// [ 2  6 10 14 ]
// [ 3  7 11 15 ]
// m[0] = (0, 1, 2, 3)
// m[1] = (4, 5, 6, 7)
// m[2] = (8, 9, 10, 11)
// m[3] = (12, 13, 14, 15)
// m * v = v.x*m[0] + v.y*m[1] + v.z*m[2] + v.w*m[3]
// v * m = (sum(v*m[0]), sum(v*m[1]), sum(v*m[2]), sum(v*m[3]))

template<typename F, size_t W, size_t H>
struct _mat{
	F data[W*H];
	static const size_t size = W*H;
	operator F*(){return data;}
	inline vec<F,H>& operator[](size_t i) const{return ((vec<F,H>*)data)[i];}
	inline F& at(size_t i) const{return data[i];}
	// Reset to the identity matrix
	void identity() requires (W == H){
		memset(data, 0, sizeof(F)*W*W);
		for(size_t i = 0; i < size; i += W+1) data[i] = 1;
	}
	F determinant() requires (W == H){
		F det = 1.0;
		F a[W*W]; memcpy(a, data, sizeof(F)*W*W);
		for(size_t i = 0; i < W; i++){
			size_t pivot = i;
			for(size_t j = i + 1; j < W; j++)
				if(abs(a[j][i]) > abs(a[pivot][i])) pivot = j;
			if(pivot != i){
				swap(a[i], a[pivot]);
				det = -det;
			}
			if(!(det *= a[i][i])) return 0;
			for(size_t j = i + 1; j < W; j++){
				F factor = a[j][i] / a[i][i];
				for(size_t k = i+1; k < W; k++) a[j][k] -= factor * a[i][k];
			}
		}
		return det;
	}
	ci vec<F,H> operator*(vec<F,W> v){
		vec<F,H> a;
		F* i = data, *end = i + W*H;
		F* vp = &v[0];
		for(; i < end; i += H){
			F x = *vp++;
			for(size_t j = 0; j < H; j++) a[j] += x * i[j];
		}
		return a;
	}
	_mat<F,W,H>& operator*=(const _mat<F,W,W>& v){
		F m[size]; memcpy(m, data, size*sizeof(F));
		F* end = m + W*H;
		F* vp = v;
		F* a = data;
		memset(a, 0, size*sizeof(F));
		for(int k = 0; k < W; k++){
			for(F* i = m; i < end; i += H){
				F x = *vp++;
				for(size_t j = 0; j < H; j++) a[j] += x * i[j];
			}
			a += H;
		}
		return *this;
	}
	template<size_t W1>
	_mat<F,W,H>& operator*=(const _mat<F,W,W1>& v) requires (W1 < W){
		F m[size]; memcpy(m, data, size*sizeof(F));
		F* end = m + W*H, *end1 = m + W1*H;
		F* vp = v;
		F* a = data;
		memset(a, 0, size*sizeof(F));
		F* k1 = m;
		for(int k = 0; k < W; k++, k1 += H){
			F* i = m;
			for(; i < end1; i += H){
				F x = *vp++;
				for(size_t j = 0; j < H; j++) a[j] += x * i[j];
			}
			for(; i < end; i += H){
				F x = k1==i;
				for(size_t j = 0; j < H; j++) a[j] += x * i[j];
			}
			a += H;
		}
		return *this;
	}
};

template<typename F, size_t W, size_t H>
struct mat: _mat<F,W,H>{};

enum PerspectiveType{ SCALES, VERT_FOV, VERT_FOV_RAD, HOR_FOV, HOR_FOV_RAD, TWO_FOVS, TWO_FOVS_RAD, AREA_FOV, AREA_FOV_RAD };

template<typename F, size_t L>
struct mat<F, 4, L>: _mat<F, 4, L>{
	// Rotate X and Z coordinates around the Y axis, such that x and z change while y remains the same. For an angle of PI/2 (90_deg), the new (1,0,0) is at (0,0,1) and the new (0,0,1) is at (-1,0,0), while (0,1,0) remains the same
	// Note that this is the same as rotateZX(-ang)
	void rotateXZ(F ang){
		F sy = sin(ang), cy = cos(ang);
		for(size_t i = 0; i < L; i++){
			F x = this->data[i], z = this->data[i+2*L];
			this->data[i] = cy*x+sy*z; this->data[i+2*L] = cy*z-sy*x;
		}
	}
	// Rotate Y and Z coordinates around the X axis, such that y and z change while x remains the same. For an angle of PI/2 (90_deg), the new (0,1,0) is at (0,0,1) and the new (0,0,1) is at (0,-1,0), while (1,0,0) remains the same
	// Note that this is the same as rotateZY(-ang)
	void rotateYZ(F ang){
		F sx = sin(ang), cx = cos(ang);
		for(size_t i = 0; i < L; i++){
			F y = this->data[i+L], z = this->data[i+2*L];
			this->data[i+L] = cx*y+sx*z; this->data[i+2*L] = cx*z-sx*y;
		}
	}
	// Rotate X and Y coordinates around the Z axis, such that x and y change while z remains the same. For an angle of PI/2 (90_deg), the new (1,0,0) is at (0,1,0) and the new (0,1,0) is at (-1,0,0), while (0,0,1) remains the same
	// Note that this is the same as rotateYX(-ang)
	void rotateXY(F ang){
		F sz = sin(ang), cz = cos(ang);
		for(size_t i = 0; i < L; i++){
			F x = this->data[i], y = this->data[i+L];
			this->data[i] = cz*x-sz*y; this->data[i+L] = cz*y+sz*x;
		}
	}
	// Rotate Z and X coordinates around the Y axis, such that z and x change while y remains the same. For an angle of PI/2 (90_deg), the new (0,0,1) is at (1,0,0) and the new (1,0,0) is at (0,0,-1), while (0,1,0) remains the same
	// Note that this is the same as rotateXZ(-ang)
	inline void rotateZX(F ang){ rotateXZ(-ang); }
	// Rotate Z and Y coordinates around the X axis, such that z and y change while x remains the same. For an angle of PI/2 (90_deg), the new (0,0,1) is at (0,1,0) and the new (0,1,0) is at (0,0,-1), while (1,0,0) remains the same
	// Note that this is the same as rotateYZ(-ang)
	inline void rotateZY(F ang){ rotateYZ(-ang); }
	// Rotate Y and X coordinates around the Z axis, such that y and x change while z remains the same. For an angle of PI/2 (90_deg), the new (0,1,0) is at (1,0,0) and the new (1,0,0) is at (0,-1,0), while (0,0,1) remains the same
	// Note that this is the same as rotateXY(-ang)
	inline void rotateYX(F ang){ rotateXY(-ang); }
	// Perform a complex multiplication where X is the real axis and Z is the imaginary axis
	// Essentially, the new (1,0,0) is at (r,0,i) by "scaling" and "rotating" only
	// Note that this is the same as multiplyZX(r, -i)
	void multiplyXZ(F r, F i){
		for(size_t j = 0; j < L; j++){
			F x = this->data[j], z = this->data[j+2*L];
			this->data[j] = r*x+i*z; this->data[j+2*L] = r*z-i*x;
		}
	}
	// Perform a complex multiplication where Y is the real axis and Z is the imaginary axis
	// Essentially, the new (0,1,0) is at (0,r,i) by "scaling" and "rotating" only
	// Note that this is the same as multiplyZY(r, -i)
	void multiplyYZ(F r, F i){
		for(size_t j = 0; j < L; j++){
			F y = this->data[j+L], z = this->data[j+2*L];
			this->data[j+L] = r*y+i*z; this->data[j+2*L] = r*z-i*y;
		}
	}
	// Perform a complex multiplication where X is the real axis and Y is the imaginary axis
	// Essentially, the new (1,0,0) is at (r,i,0) by "scaling" and "rotating" only
	// Note that this is the same as multiplyYX(r, -i)
	void multiplyXY(F r, F i){
		for(size_t j = 0; j < L; j++){
			F x = this->data[j], y = this->data[j+L];
			this->data[j] = r*x-i*y; this->data[j+L] = r*y+i*x;
		}
	}
	// Perform a complex multiplication where Z is the real axis and X is the imaginary axis
	// Essentially, the new (0,0,1) is at (i,0,r) by "scaling" and "rotating" only
	// Note that this is the same as multiplyXZ(r, -i)
	inline void multiplyZX(F r, F i){ multiplyXZ(r, -i); }
	// Perform a complex multiplication where Z is the real axis and Y is the imaginary axis
	// Essentially, the new (0,0,1) is at (0,i,r) by "scaling" and "rotating" only
	// Note that this is the same as multiplyYZ(r, -i)
	inline void multiplyZY(F r, F i){ multiplyYZ(r, -i); }
	// Perform a complex multiplication where Y is the real axis and X is the imaginary axis
	// Essentially, the new (0,1,0) is at (i,r,0) by "scaling" and "rotating" only
	// Note that this is the same as multiplyXY(r, -i)
	inline void multiplyYX(F r, F i){ multiplyXY(r, -i); }
	// Translate (offset) all coordinates by a vector such that the new (0,0,0) is at (x,y,z)
	void translate(F x, F y, F z){
		for(size_t i = 0; i < L; i++) this->data[i+3*L] += this->data[i]*x+this->data[i+L]*y+this->data[i+2*L]*z;
	}
	// Translate (offset) X coordinates by a value such that the new (0,0,0) is at (x,0,0)
	void translateX(F x){ for(size_t i = 0; i < L; i++) this->data[i+3*L] += this->data[i]*x; }
	// Translate (offset) Y coordinates by a value such that the new (0,0,0) is at (0,y,0)
	void translateY(F y){ for(size_t i = 0; i < L; i++) this->data[i+3*L] += this->data[i+L]*y; }
	// Translate (offset) Z coordinates by a value such that the new (0,0,0) is at (0,0,z)
	void translateZ(F z){ for(size_t i = 0; i < L; i++) this->data[i+3*L] += this->data[i+2*L]*z; }
	// Translate (offset) all coordinates by a vector such that the new (0,0,0) is at (to.x,to.y,to.z)
	inline void translate(vec<F,3> to){ translate(to.x,to.y,to.z); }
	// Scale all coordinates by a vector away from 0, such that the new (1,1,1) is at (x,y,z) while (0,0,0) remains (0,0,0)
	void scale(F x, F y, F z){
		for(size_t i = 0; i < L; i++){ this->data[i] *= x; this->data[i+L] *= y; this->data[i+2*L] *= z; }
	}
	// Scales X coordinates by a value away from 0, such that the new (1,1,1) is at (x,1,1) while (0,0,0) remains (0,0,0)
	void scaleX(F x){ for(size_t i = 0; i < L; i++) this->data[i] *= x; }
	// Scales Y coordinates by a value away from 0, such that the new (1,1,1) is at (1,y,1) while (0,0,0) remains (0,0,0)
	void scaleY(F y){ for(size_t i = 0; i < L; i++) this->data[i+L] *= y; }
	// Scales Z coordinates by a value away from 0, such that the new (1,1,1) is at (1,1,z) while (0,0,0) remains (0,0,0)
	void scaleZ(F z){ for(size_t i = 0; i < L; i++) this->data[i+2*L] *= z; }
	// Scale all coordinates by a vector away from 0, such that the new (1,1,1) is at (by.x,by.y,by.z) while (0,0,0) remains (0,0,0)
	inline void scale(vec<F,3> by){ scale(by.x, by.y, by.z); }
	// Scale all coordinates by a value away from 0, such that the new (1,1,1) is at (by.x,by.y,by.z) while (0,0,0) remains (0,0,0)
	void scale(F by){
		for(size_t i = 0; i < L; i++){ this->data[i] *= by; this->data[i+L] *= by; this->data[i+2*L] *= by; }
	}
	// Performs skewX(x), skewY(y) and skewZ(z) as a single accumulated skew rather than stacked on one another
	// Skews such that the new (1,0,0) is at (1,xy,xz), the new (0,1,0) is at (yx,1,yz) and the new (0,0,1) is at (zx,zy,1), while (0,0,0) remains (0,0,0)
	void skew(F yx, F zx, F xy, F zy, F xz, F yz){
		for(size_t i = 0; i < L; i++){
			F x = this->data[i], y = this->data[i+L], z = this->data[i+2*L];
			this->data[i] += xy*y+xz*z; this->data[i+L] += yx*x+yz*z; this->data[i+2*L] += zx*x+zy*y;
		}
	}
	// Performs skewYZ(yz), skewXZ(xz) and skewXY(xy) as a single accumulated skew rather than stacked on one another
	// Skews such that the new (1,0,0) is at (1,xy,xz), the new (0,1,0) is at (yx,1,yz) and the new (0,0,1) is at (zx,zy,1), while (0,0,0) remains (0,0,0)
	inline void skew2(F xy, F xz, F yx, F yz, F zx, F zy){ skew(yx, zx, xy, zy, xz, yz); }
	// Performs a skew modifying the X coordinates based on Y and Z coordinates such that the new (0,1,0) is at (yx,1,0) and the new (0,0,1) is at (zx,0,1), while (0,0,0) remains (0,0,0)
	void skewX(F yx, F zx){ for(size_t i = 0; i < L; i++){
		F x = this->data[i];
		this->data[i+L] += yx*x; this->data[i+2*L] += zx*x;
	} }
	// Performs a skew modifying the Y coordinates based on X and Z coordinates such that the new (1,0,0) is at (1,xy,0) and the new (0,0,1) is at (0,zy,1), while (0,0,0) remains (0,0,0)
	void skewY(F xy, F zy){ for(size_t i = 0; i < L; i++){
		F y = this->data[i+L];
		this->data[i] += xy*y; this->data[i+2*L] += zy*y;
	} }
	// Performs a skew modifying the Z coordinates based on X and Y coordinates such that the new (1,0,0) is at (1,0,xz) and the new (0,1,0) is at (0,1,yz), while (0,0,0) remains (0,0,0)
	void skewZ(F xz, F yz){ for(size_t i = 0; i < L; i++){
		F z = this->data[i+2*L];
		this->data[i] += xz*z; this->data[i+L] += yz*z;
	} }
	// Alias for skewX(value.x, value.y)
	inline void skewX(vec<F,2> yxzx){ skewX(yxzx.x, yxzx.y); }
	// Alias for skewY(value.x, value.y)
	inline void skewY(vec<F,2> xyzy){ skewY(xyzy.x, xyzy.y); }
	// Alias for skewZ(value.x, value.y)
	inline void skewZ(vec<F,2> xzyz){ skewZ(xzyz.x, xzyz.y); }
	// Performs a skew modifying the Y and Z coordinates based on X coordinates such that the new (1,0,0) is at (1,xy,xz), while (0,0,0) remains (0,0,0)
	void skewYZ(F xy, F xz){ for(size_t i = 0; i < L; i++){ this->data[i] += xy*this->data[i+L]+xz*this->data[i+2*L]; } }
	// Performs a skew modifying the X and Z coordinates based on Y coordinates such that the new (0,1,0) is at (yx,1,yz), while (0,0,0) remains (0,0,0)
	void skewXZ(F yx, F yz){ for(size_t i = 0; i < L; i++){ this->data[i+L] += yx*this->data[i]+yz*this->data[i+2*L]; } }
	// Performs a skew modifying the X and Y coordinates based on Z coordinates such that the new (0,0,1) is at (zx,zy,1), while (0,0,0) remains (0,0,0)
	void skewXY(F zx, F zy){ for(size_t i = 0; i < L; i++){ this->data[i+2*L] += zx*this->data[i]+zy*this->data[i+L]; } }
	// Alias for skewYZ(value.x, value.y)
	inline void skewYZ(vec<F,2> xyxz){ skewYZ(xyxz.x, xyxz.y); }
	// Alias for skewXZ(value.x, value.y)
	inline void skewXZ(vec<F,2> yxyz){ skewXZ(yxyz.x, yxyz.y); }
	// Alias for skewXY(value.x, value.y)
	inline void skewXY(vec<F,2> zxzy){ skewXY(zxzy.x, zxzy.y); }
	// Performs skewX(x), skewY(y) and skewZ(z) as a single accumulated skew rather than stacked on one another
	// Skews such that the new (1,0,0) is at (1,y[0],z[0]), the new (0,1,0) is at (x[0],1,z[1]) and the new (0,0,1) is at (x[1],y[1],1), while (0,0,0) remains (0,0,0)
	inline void skew(vec<F,2> x, vec<F,2> y, vec<F,2> z){ skew(x.x, x.y, y.x, y.y, z.x, z.y); }
	// Performs skewYZ(yz), skewXZ(xz) and skewXY(xy) as a single accumulated skew rather than stacked on one another
	// Skews such that the new (1,0,0) is at (1,yz[0],yz[1]), the new (0,1,0) is at (xz[0],1,xz[1]) and the new (0,0,1) is at (xy[0],xy[1],1), while (0,0,0) remains (0,0,0)
	inline void skew2(vec<F,2> yz, vec<F,2> xz, vec<F,2> xy){ skew(xz.x, xy.x, yz.x, xy.y, yz.y, xz.y); }

	// Returns the point that would transform to p with the current matrix. Only available on 4x3 matrices
	vec<F,3> invert(vec<F,3> p) requires (L == 3){
		F x = p.x - this->data[3*L], y = p.y - this->data[3*L+1], z = p.z - this->data[3*L+2];
		F a11 = this->data[0], a21 = this->data[1], a31 = this->data[2];
		F a12 = this->data[3], a22 = this->data[4], a32 = this->data[5];
		F a13 = this->data[6], a23 = this->data[7], a33 = this->data[8];
		F l = a22*a33-a32*a23, m = a31*a23-a21*a33, n = a21*a32-a31*a22, d = 1./(a11*l + a12*m + a13*n);
		return {
			(l * x + (a13*a32 - a33*a12) * y + (a12*a23 - a22*a13) * z) * d,
			(m * x + (a11*a33 - a31*a13) * y + (a13*a21 - a23*a11) * z) * d,
			(n * x + (a12*a31 - a32*a11) * y + (a11*a22 - a21*a12) * z) * d,
		};
	}
	// Reset the matrix to a perspective transformation
	// `SCALES` - Two field-of-view scales, one for X and one for Y, 1.0 corresponds to 45 degrees, higher values make things appear bigger by that amount but reduce visible area.
	// `VERT_FOV` - First value is field of view from the top to the bottom of the viewport in degrees, other value is Width/Height
	// `VERT_FOV_RAD` - First value is field of view from the top to the bottom of the viewport in radians, other value is Width/Height
	// `HOR_FOV` - First value is field of view from the left to the right of the viewport in degrees, other value is Width/Height
	// `HOR_FOV_RAD` - First value is field of view from the left to the right of the viewport in radians, other value is Width/Height
	// `TWO_FOVS` - First value is field of view from the left to the right of the viewport in degrees, other value is field of view from the top to the bottom of the viewport in degrees
	// `TWO_FOVS_RAD` - First value is field of view from the left to the right of the viewport in radians, other value is field of view from the top to the bottom of the viewport in radians
	// `near` - Near clipping plane. Do not set this value to 0 unless you are using a logarithmic depth buffer
	// `far` - Far clipping plane. This value along with `near` stretch the depth buffer to the desired range, so larger ranges will result in less depth precision (except whe using logarithmic depth buffers)
	template<PerspectiveType P = VERT_FOV> void perspective(F a, F b, F near, F far) requires (is_floating_point<F>::value && L >= 2){
		if constexpr(P == VERT_FOV){ F t = b; a = (b = 1/tan(a*.00872664625f)) / t; }
		else if constexpr(P == VERT_FOV){ F t = b; a = (b = 1/tan(a*.5f)) / t; }
		else if constexpr(P == HOR_FOV) b *= (a = 1/tan(a*.00872664625f));
		else if constexpr(P == HOR_FOV_RAD) b *= (a = 1/tan(a*.5f));
		else if constexpr(P == AREA_FOV){ b = (a = 1/tan(a*.00872664625f)/sqrt(b)) * b; }
		else if constexpr(P == AREA_FOV_RAD){ b = (a = 1/tan(a*.5f)/sqrt(b)) * b; }
		else if constexpr(P == TWO_FOVS){ a = 1/tan(a*.00872664625f); b = 1/tan(b*.00872664625f); }
		else if constexpr(P == TWO_FOVS_RAD){ a = 1/tan(a*.5f); b = 1/tan(b*.5f); }
		this->data[0] = a; this->data[L+1] = b;
		this->data[L] = this->data[2*L] = this->data[3*L] = this->data[1] = this->data[2*L+1] = this->data[3*L+1] = 0;
		if(L >= 3){ F a = far/(near-far); this->data[2] = this->data[L+2] = 0, this->data[2*L+2] = -a, this->data[3*L+2] = a*near; }
		if(L >= 4) this->data[3] = this->data[L+3] = this->data[3*L+3] = 0, this->data[2*L+3] = 1;
		if(L > 4) memset(this->data+4, 0, (L-4)*sizeof(F)), memset(this->data+L+4, 0, (L-4)*sizeof(F)), memset(this->data+2*L+4, 0, (L-4)*sizeof(F)), memset(this->data+3*L+4, 0, (L-4)*sizeof(F));
	}
	// Reset the matrix to an orthographic transformation, with the specified near, far, left, right, top and bottom clipping planes
	void orthographic(F left, F right, F bottom, F top, F near, F far) requires (is_floating_point<F>::value && L >= 2){
		F w = 1 / (right - left); F h = 1 / (top - bottom);
		this->data[1] = this->data[L] = this->data[2*L] = this->data[2*L+1] = 0;
		this->data[0] = 2 * w; this->data[L+1] = 2 * h;
		this->data[3*L] = -(left + right) * w; this->data[3*L+1] = -(bottom + top) * h;
		if(L >= 3){ this->data[2] = this->data[L+2] = 0; float d = -1 / (far - near); this->data[2*L+2] = -2 * d; this->data[3*L+2] = (far + near) * d; }
		if(L >= 4) this->data[3] = this->data[L+3] = this->data[2*L+3] = 0, this->data[3*L+3] = 1;
		if(L > 4) memset(this->data+4, 0, (L-4)*sizeof(F)), memset(this->data+L+4, 0, (L-4)*sizeof(F)), memset(this->data+2*L+4, 0, (L-4)*sizeof(F)), memset(this->data+3*L+4, 0, (L-4)*sizeof(F));
	}
	// Reset to the identity matrix
	void identity(){
		memset(this->data, 0, sizeof(F)*4*L);
		for(size_t i = 0; i < this->size; i += L+1) this->data[i] = 1;
	}
	// Reset to the identity matrix then perform translate(xyz) and then scale(scale)
	void reset(vec<F,3> xyz = vec<F,3>(0), vec<F,3> scale = vec<F,3>(1)) requires (L > 2){
		memset(this->data, 0, sizeof(F)*4*L);
		this->data[0] = scale.x; this->data[L+1] = scale.y; this->data[2*L+2] = scale.z;
		this->data[3*L] = xyz.x*scale.x; this->data[3*L+1] = xyz.y*scale.y; this->data[3*L+2] = xyz.z*scale.z;
		if(L > 3) this->data[3*L+3] = 1.;
	}
	// Reset to the identity matrix then perform translate(xyz) and then scale(scale)
	void reset(vec<F,2> xyz = vec<F,2>(0), vec<F,2> scale = vec<F,2>(1)) requires (L == 2){
		memset(this->data, 0, sizeof(F)*8);
		this->data[0] = scale.x; this->data[L+1] = scale.y;
		this->data[3*L] = xyz.x*scale.x; this->data[3*L+1] = xyz.y*scale.y;
	}
};

template<typename F, size_t L>
struct mat<F, 3, L>: _mat<F, 3, L>{
	// Translate (offset) all coordinates by a vector such that the new (0,0) is at (x,y)
	void translate(F x, F y){
		for(size_t i = 0; i < L; i++){ this->data[i+2*L] += this->data[i]*x+this->data[i+L]*y; }
	}
	// Translate (offset) X coordinates by a value such that the new (0,0) is at (x,0)
	void translateX(F x){ for(size_t i = 0; i < L; i++){ this->data[i+2*L] += this->data[i]*x; } }
	// Translate (offset) Y coordinates by a value such that the new (0,0) is at (0,y)
	void translateY(F y){ for(size_t i = 0; i < L; i++){ this->data[i+2*L] += this->data[i+L]*y; } }
	// Translate (offset) all coordinates by a vector such that the new (0,0) is at (to.x,to.y)
	inline void translate(vec<F,2> to){ translate(to.x,to.y); }
	// Scale all coordinates by a vector away from 0, such that the new (1,1) is at (x,y), while (0,0) remains (0,0)
	void scale(F x, F y){
		for(size_t i = 0; i < L; i++){ this->data[i] *= x; this->data[i+L] *= y; }
	}
	// Scale X coordinates by a value away from 0, such that the new (1,1) is at (x,1), while (0,0) remains (0,0)
	void scaleX(F x){ for(size_t i = 0; i < L; i++) this->data[i] *= x; }
	// Scale Y coordinates by a value away from 0, such that the new (1,1) is at (1,y), while (0,0) remains (0,0)
	void scaleY(F y){ for(size_t i = 0; i < L; i++) this->data[i+L] *= y; }
	// Scale all coordinates by a vector away from 0, such that the new (1,1) is at (by.x,by.y), while (0,0) remains (0,0)
	inline void scale(vec<F,2> by){ scale(by.x, by.y); }
	// Scale all points by a value away from 0, such that the new (1,1) is at (by,by), while (0,0) remains (0,0)
	void scale(F by){
		for(size_t i = 0; i < L; i++){ this->data[i] *= by; this->data[i+L] *= by; }
	}
	// Rotate X and Y coordinates. For an angle of PI/2 (90_deg), the new (1,0) is at (0,1) and the new (0,1) is at (-1,0), while (0,0) remains the same
	void rotate(F ang){
		F sz = sin(ang), cz = cos(ang);
		for(size_t i = 0; i < L; i++){
			F x = this->data[i], y = this->data[i+L];
			this->data[i] = cz*x-sz*y; this->data[i+L] = cz*y+sz*x;
		}
	}
	// Rotate X and Y coordinates. For an angle of PI/2 (90_deg), the new (1,0) is at (0,1) and the new (0,1) is at (-1,0), while (0,0) remains the same
	inline void rotateXY(F ang){ rotate(ang); }
	// Rotate Y and X coordinates. For an angle of PI/2 (90_deg), the new (0,1) is at (1,0) and the new (1,0) is at (0,-1), while (0,0) remains the same
	inline void rotateYX(F ang){ rotate(-ang); }
	// Perform a complex multiplication where X is the real axis and Y is the imaginary axis
	// Essentially, the new (1,0) is at (r,i) by "scaling" and "rotating" only
	void multiply(F r, F i){
		for(size_t j = 0; j < L; j++){
			F x = this->data[j], y = this->data[j+L];
			this->data[j] = r*x-i*y; this->data[j+L] = r*y+i*x;
		}
	}
	// Perform a complex multiplication where X is the real axis and Y is the imaginary axis
	// Essentially, the new (1,0) is at (r,i) by "scaling" and "rotating" only
	inline void multiplyXY(F r, F i){ multiply(r, i); }
	// Perform a complex multiplication where Y is the real axis and X is the imaginary axis
	// Essentially, the new (0,1) is at (i,r) by "scaling" and "rotating" only
	inline void multiplyYX(F r, F i){ multiply(r, -i); }
	// Performs skewX(x) and skewY(y) as a single accumulated skew rather than stacked on one another
	// Skews such that (1,0) becomes (1,xy), (0,1) becomes (yx,1) while (0,0) remains (0,0)
	void skew(F xy, F yx){
		for(size_t i = 0; i < L; i++){
			F x = this->data[i], y = this->data[i+L];
			this->data[i] += xy*y; this->data[i+L] += yx*x; 
		}
	}
	// Performs skewX(by.x) and skewY(by.y) as a single accumulated skew rather than stacked on one another
	// Skews such that (1,0) becomes (1,xy), (0,1) becomes (yx,1) while (0,0) remains (0,0)
	inline void skew(vec2 by){ skew(by.x, by.y); }
	// Performs a skew modifying the X coordinates based on Y coordinates such that the new (0,1) is at (yx,1), while (0,0) remains (0,0)
	void skewX(F yx){ for(size_t i = 0; i < L; i++){ this->data[i+L] += yx*this->data[i]; } }
	// Performs a skew modifying the Y coordinates based on X coordinates such that the new (1,0) is at (1,xy), while (0,0) remains (0,0)
	void skewY(F xy){ for(size_t i = 0; i < L; i++){ this->data[i] += xy*this->data[i+L]; } }

	// Returns the point that would transform to p with the current matrix. Only available on 3x2 matrices
	constexpr vec<F,2> invert(vec<F,2> p) requires (L == 2){
		F a = this->data[0], b = this->data[1], c = this->data[2], d = this->data[3], det = a*d-b*c;
		return {
			(p.x*(d - c) + c*this->data[5] - d*this->data[4])/det,
			(p.y*(a - b) + b*this->data[4] - a*this->data[5])/det
		};
	}

	// Reset to the identity matrix
	ci void identity(){
		memset(this->data, 0, sizeof(F)*3*L);
		for(size_t i = 0; i < this->size; i += L+1) this->data[i] = 1;
	}
	// Reset to the identity matrix then perform translate(xy) and then scale(scale)
	ci void reset(vec<F,2> xy = vec<F,2>(0), vec<F,2> scale = vec<F,2>(1)) requires (L > 1){
		memset(this->data, 0, sizeof(F)*L*3);
		this->data[0] = scale.x; this->data[L+1] = scale.y;
		this->data[2*L] = xy.x*scale.x; this->data[2*L+1] = xy.y*scale.y;
	}

	ci vec<F,L> transformPoint(vec<F,2> xy){ return this->operator*(vec<F,3>(xy,1)); }
	ci vec<F,L> transformMetric(vec<F,2> xy){ return this->operator*(vec<F,3>(xy,0)); }
};

using mat4x4 = mat<f32, 4, 4>;
using mat4x3 = mat<f32, 4, 3>;
using mat4x2 = mat<f32, 4, 2>;
using dmat4x4 = mat<f64, 4, 4>;
using dmat4x3 = mat<f64, 4, 3>;
using dmat4x2 = mat<f64, 4, 2>;
using mat3x4 = mat<f32, 3, 4>;
using mat3x3 = mat<f32, 3, 3>;
using mat3x2 = mat<f32, 3, 2>;
using dmat3x4 = mat<f64, 3, 4>;
using dmat3x3 = mat<f64, 3, 3>;
using dmat3x2 = mat<f64, 3, 2>;
using mat2x4 = mat<f32, 2, 4>;
using mat2x3 = mat<f32, 2, 3>;
using mat2x2 = mat<f32, 2, 2>;
using dmat2x4 = mat<f64, 2, 4>;
using dmat2x3 = mat<f64, 2, 3>;
using dmat2x2 = mat<f64, 2, 2>;
using mat4 = mat4x4; using dmat4 = dmat4x4;
using mat3 = mat3x3; using dmat3 = dmat3x3;
using mat2 = mat2x2; using dmat2 = dmat2x2;

struct a_lock: atomic_flag{
	a_lock() : atomic_flag ATOMIC_FLAG_INIT{}
	ci a_lock(bool set) : atomic_flag ATOMIC_FLAG_INIT{if(set) test_and_set(relaxed);}
	ci void lock(){ while(test_and_set(acq_rel)) atomic_flag::wait(true); }
	ci void set(){ test_and_set(acq_rel); }
	ci bool is_locked(){ return test(acquire); }
	ci void wait(){ if(test(acquire)) atomic_flag::wait(true); }
	ci bool try_lock(){ return !test_and_set(acq_rel); }
	ci void unlock(){ clear(release); notify_one(); }
	ci void unlock_all(){ clear(release); notify_all(); }
};

struct s_lock: atomic_flag{
	ci void lock(){ while(test_and_set(acq_rel)); }
	ci void set(){ test_and_set(acq_rel); }
	ci bool is_locked(){ return test(acquire); }
	ci void wait(){ while(test(acquire)); }
	ci bool try_lock(){ return !test_and_set(acq_rel); }
	ci void unlock(){ clear(release); }
	ci void unlock_all(){ clear(release); }
};

template<typename T> struct alignas(T) dummy{
	char value[sizeof(T)];
	ci dummy(){}
	ci dummy(const T& other){ new (value) T(other); }
	ci dummy(T&& other){ new (value) T(other); }
	ci dummy(const dummy<T>& other){ memcpy(value, other.value, sizeof(T)); }
	ci dummy(const dummy<T>&& other){ memcpy(value, other.value, sizeof(T)); }
	ci dummy<T>& operator=(const dummy<T>& other){ memcpy(value, other.value, sizeof(T)); return *this; }
	ci dummy<T>& operator=(const dummy<T>&& other){ memcpy(value, other.value, sizeof(T)); return *this; }
	
	ci T& operator*(){ return *(T*)value; }
	ci T* operator->(){ return (T*)value; }
	ci operator T&(){return *(T*)value;}
	ci operator T*(){return (T*)value;}

	template<typename... X>
	ci void construct(X... a){ new (value) T(a...); }
	template<typename... X>
	ci void replace(X... a){ ((T*)value)->~T(); new (value) T(a...); }
	ci void destruct(){ ((T*)value)->~T(); }
	ci T copy(){return *(T*)value;}
	ci T&& move(){return move(*(T*)value);}
};

template<typename T>
inline void check_addr(T* a){
	uptr b = uptr(a);
	if(b < 65536 || b > 0xffffffffffff || (b%alignof(T)) != 0) printf("\x1b[31mInvalid address 0x%zx\n", b), __builtin_trap();
}

#undef ci