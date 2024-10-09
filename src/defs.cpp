#pragma once
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
#include <type_traits>
#include <tuple>
#include <algorithm>
using namespace std;

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

template<typename F, size_t L> requires (L > 0) union vec;
template<typename F, size_t L>
union _vec_c_types{
	using y = F; using z = F; using w = F;
	using xy = vec<F,2>; using yz = vec<F, 2>; using zw = vec<F,2>;
	using xyz = vec<F,3>; using yzw = vec<F,3>;
	using rest = vec<F,L-1>;
};
template<typename F> union _vec_c_types<F, 3>{
	using y = F; using z = F; using w = F[0];
	using xy = vec<F,2>; using yz = vec<F, 2>; using zw = F;
	using xyz = F[0]; using yzw = vec<F,2>;
	using rest = vec<F,2>;
	//F x; vec<F,2> xy; union{ vec<F,2> yz; vec<F,2> __rest; struct{ F y; F z; }; };
};
template<typename F> union _vec_c_types<F, 2>{
	using y = F; using z = F[0]; using w = F[0];
	using xy = F[0]; using yz = F; using zw = F[0];
	using xyz = F[0]; using yzw = F;
	using rest = F;
	//struct{ F x; union{ F y; F __rest; }; };
};
template<typename F> union _vec_c_types<F, 1>{
	using y = F[0]; using z = F[0]; using w = F[0];
	using xy = F[1]; using yz = F[0]; using zw = F[0];
	using xyz = F; using yzw = F[0];
	using rest = F[0];
	//F x; F __rest[];
};

template<typename F, size_t R, size_t C> struct _mat;

template<typename F, size_t L> requires (L > 0)
union vec{
	using CT = _vec_c_types<F,L>;
	static const size_t length = L;
	using Type = F;
	F data[L];
	CT::xy xy; CT::xyz xyz;
	struct{ F x; union{
			CT::rest __rest; CT::yz yz; CT::yzw yzw;
			struct{ CT::y y; union{
				CT::z zw; struct{ CT::z z; CT::w w; };
			}; };
	}; };
	inline vec(F a, CT::rest b) : x(a), __rest(b){}
	template<typename... Ts>
	inline vec(F a, Ts... b) : x(a), __rest(b...){}
	template<typename X, typename... Ts> requires is_same<typename X::Type, F>::value
	inline vec(X& a, Ts... b) : x(a.x), __rest(a.__rest, b...){}
	inline vec(F a) : x(a), __rest(a){}
	inline vec() : x(0), __rest(0){}
	inline F& operator[](size_t i){return this.data[i];}
	#define OP(o, o2, d) constexpr inline vec<F,L> operator o o2(d) const{return {o x o2, o __rest o2};}
	OP(+,,) OP(-,,) OP(++,,) OP(--,,) OP(,++,int) OP(,--,int)
	#undef OP
	constexpr static inline F __sum(CT::rest& x){return x.sum();}
	constexpr static inline F __sum(F x){return x;}
	constexpr inline F sum() const{return x + __sum(__rest);};
	constexpr inline bool operator!() const{return !x && !__rest;}
	constexpr inline operator bool() const{return (bool)x && (bool)__rest;}
	constexpr inline operator F() requires (L == 1){ return x; }
	inline operator F*(){ return data; }
	#define OP(o,o2) \
		constexpr inline vec<F,L> operator o(vec<F,L> b) const{return {x o b->x, __rest o b->__rest};} \
		constexpr inline vec<F,L>& operator o2(vec<F,L> b){x o2 b->x, __rest o2 b->__rest;return *this;} \
		constexpr inline vec<F,L> operator o(F b) const{return {x o b, __rest o b};} \
		constexpr inline vec<F,L>& operator o2(F b){x o2 b, __rest o2 b;return *this;}
	OP(+, +=) OP(-, -=) OP(*, *=) OP(/, /=) OP(%, %=) OP(&, &=) OP(|, |=) OP(^, ^=) OP(<<, <<=) OP(>>, >>=)
	#undef OP
	template<size_t O> vec<F,O> operator*(_mat<F,O,L>& m){
		vec<F,O> a;
		F* b = a, *i = m.data, *end = i + O*L;
		for(; i < end; i += L){
			F acc = 0;
			for(size_t j = 0; j < L; j++) acc += data[j] * i[j];
			*b++ = acc;
		}
		return a;
	}
	vec<F,L>& operator*=(_mat<F,L,L>& m){
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


using vec2 = vec<f32,2>; using dvec2 = vec<f64,2>; using ivec2 = vec<i32,2>; using uvec2 = vec<u32,2>;
using vec3 = vec<f32,3>; using dvec3 = vec<f64,3>; using ivec3 = vec<i32,3>; using uvec3 = vec<u32,3>;
using vec4 = vec<f32,4>; using dvec4 = vec<f64,4>; using ivec4 = vec<i32,4>; using uvec4 = vec<u32,4>;
static_assert(offsetof(vec2,z) == offsetof(vec2,w));


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

template<typename F, size_t R, size_t C>
struct _mat{
	F data[R*C];
	static const size_t size = R*C;
	operator F*(){return data;}
	inline vec<F,C>& operator[](size_t i) const{return ((vec<F,C>*)data)[i];}
	inline F& at(size_t i) const{return data[i];}
	vec<F,C> operator*(vec<F,R>& v){
		vec<F,C> a;
		F* i = data, *end = i + R*C;
		F* vp = &v[0];
		for(; i < end; i += C){
			F x = *vp++;
			for(size_t j = 0; j < C; j++) a[j] += x * i[j];
		}
		return a;
	}
	_mat<F,R,C>& operator*=(_mat<F,R,R>& v){
		F m[size]; memcpy(m, data, size*sizeof(F));
		F* i = m, *end = m + R*C;
		F* vp = v;
		F* a = data;
		memset(a, 0, size*sizeof(F));
		for(int k = 0; k < R; k++){
			for(; i < end; i += C){
				F x = *vp++;
				for(size_t j = 0; j < C; j++) a[j] += x * i[j];
			}
			a += C;
		}
		return *this;
	}
};

template<typename F, size_t L> struct _mat<F, L, L>{
	F data[L*L];
	static const size_t size = L*L;
	operator F*(){return data;}
	inline vec<F,L>& operator[](size_t i) const{return ((vec<F,L>*)this)[i];}
	inline F& at(size_t i) const{return data[i];}
	void identity(){
		memset(data, 0, sizeof(F)*L*L);
		for(size_t i = 0; i < size; i += L+1) data[i] = 1;
	}
	F determinant(){
		F det = 1.0;
		F a[L*L]; memcpy(a, data, sizeof(F)*L*L);
		for(size_t i = 0; i < L; i++){
			size_t pivot = i;
			for(size_t j = i + 1; j < L; j++)
				if(abs(a[j][i]) > abs(a[pivot][i])) pivot = j;
			if(pivot != i){
				swap(a[i], a[pivot]);
				det = -det;
			}
			if(!(det *= a[i][i])) return 0;
			for(size_t j = i + 1; j < L; j++){
				F factor = a[j][i] / a[i][i];
				for(size_t k = i+1; k < L; k++) a[j][k] -= factor * a[i][k];
			}
		}
		return det;
	}
};

template<typename F, size_t R, size_t C>
struct mat: _mat<F,R,C>{};

enum PerspectiveType{ SCALES, VERT_FOV, VERT_FOV_RAD, HOR_FOV, HOR_FOV_RAD, TWO_FOVS, TWO_FOVS_RAD, AREA_FOV, AREA_FOV_RAD };

template<typename F, size_t L>
struct mat<F, 4, L>: _mat<F, 4, L>{
	// Rotate all coordinates around the Y axis, such that x and z change while y remains the same
	void rotateXZ(F y){
		F x,z, sy = sin(y), cy = cos(y);
		for(size_t i = 0; i < L; i++){ x = this->data[i]; z = this->data[i+2*L]; this->data[i] = cy*x+sy*z; this->data[i+2*L] = cy*z-sy*x; }
	}
	// Rotate all coordinates around the X axis, such that y and z change while x remains the same
	void rotateYZ(F x){
		F y,z, sx = sin(x), cx = cos(x);
		for(size_t i = 0; i < L; i++){ y = this->data[i+L]; z = this->data[i+2*L]; this->data[i+L] = cx*y+sx*z; this->data[i+2*L] = cx*z-sx*y; }
	}
	// Rotate all coordinates around the Z axis, such that x and y change while x remains the same
	void rotateXY(F z){
		F x,y, sz = sin(z), cz = cos(z);
		for(size_t i = 0; i < L; i++){ y = this->data[i+L]; x = this->data[i]; this->data[i+L] = cz*y+sz*x; this->data[i] = cz*x-sz*y; }
	}
	// Translate (offset) all coordinates by a value such that (x,y,z) becomes (x + value.x, y + value.y, z + value.z)
	void translate(F x, F y, F z){
		for(size_t i = 0; i < L; i++){ this->data[i+3*L] += (this->data[i]*x+this->data[i+L]*y+this->data[i+2*L]*z); }
	}
	inline void translate(vec<F,3> by){ translate(by.x,by.y,by.z); }
	// Scale all coordinates by a value away from 0, such that (x,y,z) becomes (x * value.x, y * value.y, z * value.z)
	void scale(vec<F,3> by){
		for(size_t i = 0; i < L; i++){ this->data[i] *= by.x; this->data[i+L] *= by.y; this->data[i+2*L] *= by.z; }
	}
	// Scale all points by a value away from 0, while (0,0,0) remains (0,0,0)
	void scale(F by){
		for(size_t i = 0; i < L; i++){ this->data[i] *= by; this->data[i+L] *= by; this->data[i+2*L] *= by; }
	}
	// Skew such that (1,0,0) becomes (1,xy,xz), (0,1,0) becomes (yx,1,yz) and (0,0,1) becomes (zx,zy,1), while (0,0,0) remains (0,0,0)
	void skew(F xz, F xy, F yx, F yz, F zx, F zy){
		F x,y,z;
		for(size_t i = 0; i < L; i++){ x=this->data[i];y=this->data[i+L];z=this->data[i+2*L];this->data[i] += xy*y+xz*z; this->data[i+L] += yx*x+yz*z; this->data[i+2*L] += zx*x+zy*y; }
	}
	// Skew such that (1,0,0) becomes (1,x,x), (0,1,0) becomes (y,1,y) and (0,0,1) becomes (z,z,1), while (0,0,0) remains (0,0,0)
	void skew(vec<F,2> x, vec<F,2> y, vec<F,2> z){ skew(x.x, x.y, y.x, y.y, z.x, z.y); }
	// Reset the matrix to a perspective transformation
	// `fovDeg` - Field of view from the top to the bottom of the viewport in degrees
	// `ratio` - Width/Height. Setting this to the wrong value will result in stretched images (consider that the FOV from the left to the right is not the same as the FOV from the top to the bottom)
	
	// Set perspective camera
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
		if(L >= 3){ F a = far/(near-far); this->data[2] = this->data[6] = 0, this->data[10] = a, this->data[14] = a*near; }
		if(L >= 4) this->data[3] = this->data[7] = this->data[15] = 0, this->data[11] = -1;
		if(L > 4) memset(this->data+4, 0, (L-4)*sizeof(F)), memset(this->data+L+4, 0, (L-4)*sizeof(F)), memset(this->data+2*L+4, 0, (L-4)*sizeof(F)), memset(this->data+3*L+4, 0, (L-4)*sizeof(F));
	}
	
	void orthographic(F left, F right, F bottom, F top, F near, F far) requires (is_floating_point<F>::value && L >= 2){
		F w = 1 / (right - left); F h = 1 / (top - bottom);
		this->data[1] = this->data[L] = this->data[2*L] = this->data[2*L+1] = 0;
		this->data[0] = 2 * w; this->data[L+1] = 2 * h;
		this->data[3*L] = -(left + right) * w; this->data[3*L+1] = -(bottom + top) * h;
		if(L >= 3){ this->data[2] = this->data[L+2] = 0; float d = -1 / (far - near); this->data[2*L+2] = 2 * d; this->data[3*L+2] = (far + near) * d; }
		if(L >= 4) this->data[3] = this->data[L+3] = this->data[2*L+3] = 0, this->data[3*L+3] = 1;
		if(L > 4) memset(this->data+4, 0, (L-4)*sizeof(F)), memset(this->data+L+4, 0, (L-4)*sizeof(F)), memset(this->data+2*L+4, 0, (L-4)*sizeof(F)), memset(this->data+3*L+4, 0, (L-4)*sizeof(F));
	}
	void identity(){
		memset(this->data, 0, sizeof(F)*4*L);
		for(size_t i = 0; i < this->size; i += L+1) this->data[i] = 1;
	}
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