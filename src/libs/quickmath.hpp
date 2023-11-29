/* ------------------------------------------------------------------------
 *
 * quickmath.hpp
 * author: Daniel Elwell (2022)
 * license: MIT
 * description: a single-header library for common vector, matrix, and quaternion math
 * functions designed for games and graphics programming.
 * 
 * ------------------------------------------------------------------------
 * 
 * all the types and functions provided by this library are under the the "qm" namespace
 * 
 * if you wish not to use SSE3 intrinsics (if they are not supported for example),
 * change the macro on line 101 to "#define QM_USE_SSE 0"
 * 
 * if you wish not to include <iostream> in your project, simply change the macro on line 109
 * to "#define QM_INCLUDE_IOSTREAM 0" and any iostream related functions will not be compiled
 * 
 * to disable the need to link with the C runtime library, change the macros beginning
 * on line 116 and the #include on line 114 to the appropirate functions/files
 * 
 * ------------------------------------------------------------------------
 * 
 * the following functions are defined:
 * (vecn means a vector of dimension, 2, 3, or 4, named vec2, vec3, and vec4)
 * (matn means a matrix of dimensions 3x3 or 4x4, named mat3 and mat4)
 * 
 * vecn       dot                        (vecn v1, vecn v2);
 * vec3       cross                      (vec3 v1, vec3 v2);
 * float      length                     (vecn v);
 * vecn       normalize                  (vecn v);
 * float      distance                   (vecn v1, vecn v2);
 * vecn       min                        (vecn v1, vecn v2);
 * vecn       max                        (vecn v1, vecn v2);
 * 
 * matn       matn_identity              ();
 * matn       transpose                  (matn m);
 * matn       inverse                    (matn m);
 * 
 * mat3       translate                  (vec2 t);
 * mat4       translate                  (vec3 t);
 * mat3       scale                      (vec2 s);
 * mat4       scale                      (vec3 s);
 * mat3       rotate                     (float angle);
 * mat4       rotate                     (vec3 axis, float angle);
 * mat4       rotate                     (vec3 euler);
 * mat3       top_left                   (mat4 m);
 *
 * mat4       perspective                (float fov, float aspect, float near, float far);
 * mat4       orthographic               (float left, float right, float bot, float top, float near, float far);
 * mat4       look                       (vec3 pos, vec3 dir   , vec3 up);
 * mat4       lookat                     (vec3 pos, vec3 target, vec3 up);
 * 
 * quaternion quaternion_identity        ();
 * quaternion dot                        (quaternion q1, quaternion q2);
 * float      length                     (quaternion q);
 * quaternion normalize                  (quaternion q);
 * quaternion conjugate                  (quaternion q);
 * quaternion inverse                    (quaternion q);
 * quaternion slerp                      (quaternion q1, quaternion q2, float a);
 * quaternion quaternion_from_axis_angle (vec3 axis, float angle);
 * quaternion quaternion_from_euler      (vec3 angles);
 * mat4       quaternion_to_mat4         (quaternion q);
 * 
 * the following operators are defined:
 * (vecn means a vector of dimension, 2, 3, or 4, named vec2, vec3, and vec4)
 * (matn means a matrix of dimensions 3x3 or 4x4, named mat3 and mat4)
 * 
 * vecn + vecn              -> vecn
 * vecn - vecn              -> vecn
 * vecn * vecn              -> vecn
 * vecn / vecn              -> vecn
 * vecn * float             -> vecn
 * float * vecn             -> vecn
 * vecn / float             -> vecn
 * float / vecn             -> vecn
 * vecn == vecn             -> bool
 * vecn != vecn             -> bool
 * 
 * matn + matn              -> matn
 * matn - matn              -> matn
 * matn * matn              -> matn
 * matn * vecn              -> vecn
 * 
 * quaternion + quaternion  -> quaternion
 * quaternion - quaternion  -> quaternion
 * quaternion * quaternion  -> quaternion
 * quaternion * float       -> quaternion
 * float * quaternion       -> quaternion
 * quaternion / float       -> quaternion
 * float / quaternion       -> quaternion
 * quaternion == quaternion -> bool
 * quaternion != quaternion -> bool
 */

#ifndef QM_MATH_H
#define QM_MATH_H

//if you wish NOT to use SSE3 SIMD intrinsics, simply change the
//#define to 0
#define QM_USE_SSE 1
#if QM_USE_SSE
	#include <xmmintrin.h>
	#include <pmmintrin.h>
#endif

//if you wish NOT to include iostream, simply change the
//#define to 0
#define QM_INCLUDE_IOSTREAM 1
#if QM_INCLUDE_IOSTREAM
	#include <iostream>
#endif

//if you wish to not use any of the CRT functions, you must #define your
//own versions of the below functions and #include the appropriate header
#include <math.h>

#define QM_SQRTF sqrtf
#define QM_SINF  sinf
#define QM_COSF  cosf
#define QM_TANF  tanf
#define QM_ACOSF acosf

namespace qm
{

//----------------------------------------------------------------------//
//STRUCT DEFINITIONS:

//a 2-dimensional vector of floats
union vec2
{
	float v[2] = {};
	struct{ float x, y; };
	struct{ float w, h; };

	vec2() {};
	vec2(float _x, float _y) { x = _x, y = _y; };

	inline float& operator[](size_t i) { return v[i]; };
};

//a 3-dimensional vector of floats
union vec3
{
	float v[3] = {};
	struct{ float x, y, z; };
	struct{ float w, h, d; };
	struct{ float r, g, b; };

	vec3() {};
	vec3(float _x, float _y, float _z) { x = _x, y = _y, z = _z; };
	vec3(vec2 _xy, float _z) { x = _xy.x, y = _xy.y, z = _z; };
	vec3(float _x, vec3 _yz) { x = _x, y = _yz.x, z = _yz.y; };

	inline float& operator[](size_t i) { return v[i]; };
};

//a 4-dimensional vector of floats
union vec4
{
	float v[4] = {};
	struct{ float x, y, z, w; };
	struct{ float r, g, b, a; };

	#if QM_USE_SSE

	__m128 packed;

	#endif

	vec4() {};
	vec4(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
	vec4(vec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
	vec4(float _x, vec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
	vec4(vec2 _xy, vec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

	inline float& operator[](size_t i) { return v[i]; };
};

//-----------------------------//
//matrices are column-major

union mat3
{
	float m[3][3] = {};
	vec3 v[3];

	mat3() {};

	inline vec3& operator[](size_t i) { return v[i]; };
};

union mat4
{
	float m[4][4] = {};
	vec4 v[4];

	#if QM_USE_SSE

	__m128 packed[4]; //array of columns

	#endif

	mat4() {};

	inline vec4& operator[](size_t i) { return v[i]; };
};

//-----------------------------//

union quaternion
{
	float q[4] = {};
	struct{ float x, y, z, w; };

	#if QM_USE_SSE

	__m128 packed;

	#endif

	quaternion() {};
	quaternion(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
	quaternion(vec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
	quaternion(float _x, vec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
	quaternion(vec2 _xy, vec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

	inline float operator[](size_t i) { return q[i]; };
};

//----------------------------------------------------------------------//
//HELPER FUNCS:

#define QM_MIN(x, y) ((x) < (y) ? (x) : (y))
#define QM_MAX(x, y) ((x) > (y) ? (x) : (y))
#define QM_ABS(x) ((x) > 0 ? (x) : -(x))

inline float rad_to_deg(float rad)
{
	return rad * 57.2957795131f;
}

inline float deg_to_rad(float deg)
{
	return deg * 0.01745329251f;
}

#if QM_USE_SSE

inline __m128 mat4_mult_column_sse(__m128 c1, mat4 m2)
{
	__m128 result;

	result =                    _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(0, 0, 0, 0)), m2.packed[0]);
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(1, 1, 1, 1)), m2.packed[1]));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(2, 2, 2, 2)), m2.packed[2]));
	result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(3, 3, 3, 3)), m2.packed[3]));

	return result;
}

#endif

//----------------------------------------------------------------------//
//VECTOR FUNCTIONS:

#if QM_INCLUDE_IOSTREAM

//output:

inline std::ostream& operator<<(std::ostream& os, const vec2& v)
{
	os << v.x << ", " << v.y;
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const vec3& v)
{
	os << v.x << ", " << v.y << ", " << v.z;
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const vec4& v)
{
	os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
	return os;
}

//input:

inline std::istream& operator>>(std::istream& is, vec2& v)
{
	is >> v.x >> v.y;
	return is;
}

inline std::istream& operator>>(std::istream& is, vec3& v)
{
	is >> v.x >> v.y >> v.z;
	return is;
}

inline std::istream& operator>>(std::istream& is, vec4& v)
{
	is >> v.x >> v.y >> v.z >> v.w;
	return is;
}

#endif

//addition:

inline vec2 operator+(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;

	return result;
}

inline vec3 operator+(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;

	return result;
}

inline vec4 operator+(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_add_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	result.w = v1.w + v2.w;

	#endif

	return result;
}

//subtraction:

inline vec2 operator-(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;

	return result;
}

inline vec3 operator-(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;

	return result;
}

inline vec4 operator-(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_sub_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	result.w = v1.w - v2.w;

	#endif

	return result;
}

//multiplication:

inline vec2 operator*(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;

	return result;
}

inline vec3 operator*(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;
	result.z = v1.z * v2.z;

	return result;
}

inline vec4 operator*(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_mul_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x * v2.x;
	result.y = v1.y * v2.y;
	result.z = v1.z * v2.z;
	result.w = v1.w * v2.w;

	#endif

	return result;
}

//division:

inline vec2 operator/(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;

	return result;
}

inline vec3 operator/(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;
	result.z = v1.z / v2.z;

	return result;
}

inline vec4 operator/(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_div_ps(v1.packed, v2.packed);

	#else

	result.x = v1.x / v2.x;
	result.y = v1.y / v2.y;
	result.z = v1.z / v2.z;
	result.w = v1.w / v2.w;

	#endif

	return result;
}

//scalar multiplication:

inline vec2 operator*(const vec2& v, float s)
{
	vec2 result;

	result.x = v.x * s;
	result.y = v.y * s;

	return result;
}

inline vec3 operator*(const vec3& v, float s)
{
	vec3 result;

	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;

	return result;
}

inline vec4 operator*(const vec4& v, float s)
{
	vec4 result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_mul_ps(v.packed, scale);

	#else

	result.x = v.x * s;
	result.y = v.y * s;
	result.z = v.z * s;
	result.w = v.w * s;

	#endif

	return result;
}

inline vec2 operator*(float s, const vec2& v)
{
	return v * s;
}

inline vec3 operator*(float s, const vec3& v)
{
	return v * s;
}

inline vec4 operator*(float s, const vec4& v)
{
	return v * s;
}

//scalar division:

inline vec2 operator/(const vec2& v, float s)
{
	vec2 result;

	result.x = v.x / s;
	result.y = v.y / s;

	return result;
}

inline vec3 operator/(const vec3& v, float s)
{
	vec3 result;

	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;

	return result;
}

inline vec4 operator/(const vec4& v, float s)
{
	vec4 result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_div_ps(v.packed, scale);

	#else

	result.x = v.x / s;
	result.y = v.y / s;
	result.z = v.z / s;
	result.w = v.w / s;

	#endif

	return result;
}

inline vec2 operator/(float s, const vec2& v)
{
	vec2 result;

	result.x = s / v.x;
	result.y = s / v.y;

	return result;
}

inline vec3 operator/(float s, const vec3& v)
{
	vec3 result;

	result.x = s / v.x;
	result.y = s / v.y;
	result.z = s / v.z;

	return result;
}

inline vec4 operator/(float s, const vec4& v)
{
	vec4 result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_div_ps(scale, v.packed);

	#else

	result.x = s / v.x;
	result.y = s / v.y;
	result.z = s / v.z;
	result.w = s / v.w;

	#endif

	return result;
}

//dot product:

inline float dot(const vec2& v1, const vec2& v2)
{
	float result;

	result = v1.x * v2.x + v1.y * v2.y;

	return result;
}

inline float dot(const vec3& v1, const vec3& v2)
{
	float result;

	result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

	return result;
}

inline float dot(const vec4& v1, const vec4& v2)
{
	float result;

	#if QM_USE_SSE

	__m128 r = _mm_mul_ps(v1.packed, v2.packed);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	_mm_store_ss(&result, r);

	#else

	result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;

	#endif

	return result;
}

//cross product

inline vec3 cross(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = (v1.y * v2.z) - (v1.z * v2.y);
	result.y = (v1.z * v2.x) - (v1.x * v2.z);
	result.z = (v1.x * v2.y) - (v1.y * v2.x);

	return result;
}

//length:

inline float length(const vec2& v)
{
	float result;

	result = QM_SQRTF(dot(v, v));

	return result;
}

inline float length(const vec3& v)
{
	float result;

	result = QM_SQRTF(dot(v, v));

	return result;
}

inline float length(const vec4& v)
{
	float result;

	result = QM_SQRTF(dot(v, v));

	return result;
}

//normalize:

inline vec2 normalize(const vec2& v)
{
	vec2 result;

	float len = length(v);
	if(len != 0.0f)
		result = v / len;

	return result;
}

inline vec3 normalize(const vec3& v)
{
	vec3 result;

	float len = length(v);
	if(len != 0.0f)
		result = v / len;

	return result;
}

inline vec4 normalize(const vec4& v)
{
	vec4 result;

	float len = length(v);
		result = v / len;

	return result;
}

//distance:

inline float distance(const vec2& v1, const vec2& v2)
{
	float result;

	vec2 to = v1 - v2;
	result = length(to);

	return result;
}

inline float distance(const vec3& v1, const vec3& v2)
{
	float result;

	vec3 to = v1 - v2;
	result = length(to);

	return result;
}

inline float distance(const vec4& v1, const vec4& v2)
{
	float result;

	vec4 to = v1 - v2;
	result = length(to);

	return result;
}

//equality:

inline bool operator==(const vec2& v1, const vec2& v2)
{
	bool result;

	result = (v1.x == v2.x) && (v1.y == v2.y); 

	return result;	
}

inline bool operator==(const vec3& v1, const vec3& v2)
{
	bool result;

	result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z); 

	return result;	
}

inline bool operator==(const vec4& v1, const vec4& v2)
{
	bool result;

	//TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
	result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w); 

	return result;	
}

inline bool operator!=(const vec2& v1, const vec2& v2)
{
	bool result;

	result = (v1.x != v2.x) || (v1.y != v2.y);

	return result;
}

inline bool operator!=(const vec3& v1, const vec3& v2)
{
	bool result;

	result = (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z);

	return result;
}

inline bool operator!=(const vec4& v1, const vec4& v2)
{
	bool result;

	result = (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w);

	return result;
}

//min:

inline vec2 min(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);

	return result;
}

inline vec3 min(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);
	result.z = QM_MIN(v1.z, v2.z);

	return result;
}

inline vec4 min(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_min_ps(v1.packed, v2.packed);

	#else

	result.x = QM_MIN(v1.x, v2.x);
	result.y = QM_MIN(v1.y, v2.y);
	result.z = QM_MIN(v1.z, v2.z);
	result.w = QM_MIN(v1.w, v2.w);

	#endif

	return result;
}

//max:

inline vec2 max(const vec2& v1, const vec2& v2)
{
	vec2 result;

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);

	return result;
}

inline vec3 max(const vec3& v1, const vec3& v2)
{
	vec3 result;

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);
	result.z = QM_MAX(v1.z, v2.z);

	return result;
}

inline vec4 max(const vec4& v1, const vec4& v2)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = _mm_max_ps(v1.packed, v2.packed);

	#else

	result.x = QM_MAX(v1.x, v2.x);
	result.y = QM_MAX(v1.y, v2.y);
	result.z = QM_MAX(v1.z, v2.z);
	result.w = QM_MAX(v1.w, v2.w);

	#endif

	return result;
}

//----------------------------------------------------------------------//
//MATRIX FUNCTIONS:

#if QM_INCLUDE_IOSTREAM

//output:

inline std::ostream& operator<<(std::ostream& os, const mat3& m)
{
	os << m.v[0] << std::endl << m.v[1] << std::endl << m.v[2];
	return os;
}

inline std::ostream& operator<<(std::ostream& os, const mat4& m)
{
	os << m.v[0] << std::endl << m.v[1] << std::endl << m.v[2] << std::endl << m.v[3];
	return os;
}

//input:

inline std::istream& operator>>(std::istream& is, mat3& m)
{
	is >> m.v[0] >> m.v[1] >> m.v[2];
	return is;
}

inline std::istream& operator>>(std::istream& is, mat4& m)
{
	is >> m.v[0] >> m.v[1] >> m.v[2] >> m.v[3];
	return is;
}

#endif

//initialization:

inline mat3 mat3_identity()
{
	mat3 result;

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;

	return result;
}

inline mat4 mat4_identity()
{
	mat4 result;

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

//addition:

inline mat3 operator+(const mat3& m1, const mat3& m2)
{
	mat3 result;

	result.m[0][0] = m1.m[0][0] + m2.m[0][0];
	result.m[0][1] = m1.m[0][1] + m2.m[0][1];
	result.m[0][2] = m1.m[0][2] + m2.m[0][2];
	result.m[1][0] = m1.m[1][0] + m2.m[1][0];
	result.m[1][1] = m1.m[1][1] + m2.m[1][1];
	result.m[1][2] = m1.m[1][2] + m2.m[1][2];
	result.m[2][0] = m1.m[2][0] + m2.m[2][0];
	result.m[2][1] = m1.m[2][1] + m2.m[2][1];
	result.m[2][2] = m1.m[2][2] + m2.m[2][2];

	return result;
}

inline mat4 operator+(const mat4& m1, const mat4& m2)
{
	mat4 result;

	#if QM_USE_SSE

	result.packed[0] = _mm_add_ps(m1.packed[0], m2.packed[0]);
	result.packed[1] = _mm_add_ps(m1.packed[1], m2.packed[1]);
	result.packed[2] = _mm_add_ps(m1.packed[2], m2.packed[2]);
	result.packed[3] = _mm_add_ps(m1.packed[3], m2.packed[3]);

	#else

	result.m[0][0] = m1.m[0][0] + m2.m[0][0];
	result.m[0][1] = m1.m[0][1] + m2.m[0][1];
	result.m[0][2] = m1.m[0][2] + m2.m[0][2];
	result.m[0][3] = m1.m[0][3] + m2.m[0][3];
	result.m[1][0] = m1.m[1][0] + m2.m[1][0];
	result.m[1][1] = m1.m[1][1] + m2.m[1][1];
	result.m[1][2] = m1.m[1][2] + m2.m[1][2];
	result.m[1][3] = m1.m[1][3] + m2.m[1][3];
	result.m[2][0] = m1.m[2][0] + m2.m[2][0];
	result.m[2][1] = m1.m[2][1] + m2.m[2][1];
	result.m[2][2] = m1.m[2][2] + m2.m[2][2];
	result.m[2][3] = m1.m[2][3] + m2.m[2][3];
	result.m[3][0] = m1.m[3][0] + m2.m[3][0];
	result.m[3][1] = m1.m[3][1] + m2.m[3][1];
	result.m[3][2] = m1.m[3][2] + m2.m[3][2];
	result.m[3][3] = m1.m[3][3] + m2.m[3][3];

	#endif

	return result;
}

//subtraction:

inline mat3 operator-(const mat3& m1, const mat3& m2)
{
	mat3 result;

	result.m[0][0] = m1.m[0][0] - m2.m[0][0];
	result.m[0][1] = m1.m[0][1] - m2.m[0][1];
	result.m[0][2] = m1.m[0][2] - m2.m[0][2];
	result.m[1][0] = m1.m[1][0] - m2.m[1][0];
	result.m[1][1] = m1.m[1][1] - m2.m[1][1];
	result.m[1][2] = m1.m[1][2] - m2.m[1][2];
	result.m[2][0] = m1.m[2][0] - m2.m[2][0];
	result.m[2][1] = m1.m[2][1] - m2.m[2][1];
	result.m[2][2] = m1.m[2][2] - m2.m[2][2];

	return result;
}

inline mat4 operator-(const mat4& m1, const mat4& m2)
{
	mat4 result;

	#if QM_USE_SSE

	result.packed[0] = _mm_sub_ps(m1.packed[0], m2.packed[0]);
	result.packed[1] = _mm_sub_ps(m1.packed[1], m2.packed[1]);
	result.packed[2] = _mm_sub_ps(m1.packed[2], m2.packed[2]);
	result.packed[3] = _mm_sub_ps(m1.packed[3], m2.packed[3]);

	#else

	result.m[0][0] = m1.m[0][0] - m2.m[0][0];
	result.m[0][1] = m1.m[0][1] - m2.m[0][1];
	result.m[0][2] = m1.m[0][2] - m2.m[0][2];
	result.m[0][3] = m1.m[0][3] - m2.m[0][3];
	result.m[1][0] = m1.m[1][0] - m2.m[1][0];
	result.m[1][1] = m1.m[1][1] - m2.m[1][1];
	result.m[1][2] = m1.m[1][2] - m2.m[1][2];
	result.m[1][3] = m1.m[1][3] - m2.m[1][3];
	result.m[2][0] = m1.m[2][0] - m2.m[2][0];
	result.m[2][1] = m1.m[2][1] - m2.m[2][1];
	result.m[2][2] = m1.m[2][2] - m2.m[2][2];
	result.m[2][3] = m1.m[2][3] - m2.m[2][3];
	result.m[3][0] = m1.m[3][0] - m2.m[3][0];
	result.m[3][1] = m1.m[3][1] - m2.m[3][1];
	result.m[3][2] = m1.m[3][2] - m2.m[3][2];
	result.m[3][3] = m1.m[3][3] - m2.m[3][3];

	#endif

	return result;
}

//multiplication:

inline mat3 operator*(const mat3& m1, const mat3& m2)
{
	mat3 result;

	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2];
	result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2];
	result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2];
	result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2];
	result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2];
	result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2];
	result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2];
	result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2];
	result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2];

	return result;
}

inline mat4 operator*(const mat4& m1, const mat4& m2)
{
	mat4 result;

	#if QM_USE_SSE

	result.packed[0] = mat4_mult_column_sse(m2.packed[0], m1);
	result.packed[1] = mat4_mult_column_sse(m2.packed[1], m1);
	result.packed[2] = mat4_mult_column_sse(m2.packed[2], m1);
	result.packed[3] = mat4_mult_column_sse(m2.packed[3], m1);

	#else

	result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2] + m1.m[3][0] * m2.m[0][3];
	result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2] + m1.m[3][1] * m2.m[0][3];
	result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2] + m1.m[3][2] * m2.m[0][3];
	result.m[0][3] = m1.m[0][3] * m2.m[0][0] + m1.m[1][3] * m2.m[0][1] + m1.m[2][3] * m2.m[0][2] + m1.m[3][3] * m2.m[0][3];
	result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2] + m1.m[3][0] * m2.m[1][3];
	result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2] + m1.m[3][1] * m2.m[1][3];
	result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2] + m1.m[3][2] * m2.m[1][3];
	result.m[1][3] = m1.m[0][3] * m2.m[1][0] + m1.m[1][3] * m2.m[1][1] + m1.m[2][3] * m2.m[1][2] + m1.m[3][3] * m2.m[1][3];
	result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2] + m1.m[3][0] * m2.m[2][3];
	result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2] + m1.m[3][1] * m2.m[2][3];
	result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2] + m1.m[3][2] * m2.m[2][3];
	result.m[2][3] = m1.m[0][3] * m2.m[2][0] + m1.m[1][3] * m2.m[2][1] + m1.m[2][3] * m2.m[2][2] + m1.m[3][3] * m2.m[2][3];
	result.m[3][0] = m1.m[0][0] * m2.m[3][0] + m1.m[1][0] * m2.m[3][1] + m1.m[2][0] * m2.m[3][2] + m1.m[3][0] * m2.m[3][3];
	result.m[3][1] = m1.m[0][1] * m2.m[3][0] + m1.m[1][1] * m2.m[3][1] + m1.m[2][1] * m2.m[3][2] + m1.m[3][1] * m2.m[3][3];
	result.m[3][2] = m1.m[0][2] * m2.m[3][0] + m1.m[1][2] * m2.m[3][1] + m1.m[2][2] * m2.m[3][2] + m1.m[3][2] * m2.m[3][3];
	result.m[3][3] = m1.m[0][3] * m2.m[3][0] + m1.m[1][3] * m2.m[3][1] + m1.m[2][3] * m2.m[3][2] + m1.m[3][3] * m2.m[3][3];

	#endif

	return result;
}

inline vec3 operator*(const mat3& m, const vec3& v)
{
	vec3 result;

	result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z;
	result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z;
	result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z;

	return result;
}

inline vec4 operator*(const mat4& m, const vec4& v)
{
	vec4 result;

	#if QM_USE_SSE

	result.packed = mat4_mult_column_sse(v.packed, m);

	#else

	result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;
	result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;
	result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;
	result.w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

	#endif

	return result;
}

//transpose:

inline mat3 transpose(const mat3& m)
{
	mat3 result;

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[1][0];
	result.m[0][2] = m.m[2][0];
	result.m[1][0] = m.m[0][1];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[2][1];
	result.m[2][0] = m.m[0][2];
	result.m[2][1] = m.m[1][2];
	result.m[2][2] = m.m[2][2];

	return result;
}

inline mat4 transpose(const mat4& m)
{
	mat4 result = m;

	#if QM_USE_SSE

	_MM_TRANSPOSE4_PS(result.packed[0], result.packed[1], result.packed[2], result.packed[3]);

	#else

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[1][0];
	result.m[0][2] = m.m[2][0];
	result.m[0][3] = m.m[3][0];
	result.m[1][0] = m.m[0][1];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[2][1];
	result.m[1][3] = m.m[3][1];
	result.m[2][0] = m.m[0][2];
	result.m[2][1] = m.m[1][2];
	result.m[2][2] = m.m[2][2];
	result.m[2][3] = m.m[3][2];
	result.m[3][0] = m.m[0][3];
	result.m[3][1] = m.m[1][3];
	result.m[3][2] = m.m[2][3];
	result.m[3][3] = m.m[3][3];

	#endif

	return result;
}

//inverse:

inline mat3 inverse(const mat3& m)
{
	mat3 result;

	float det;
  	float a = m.m[0][0], b = m.m[0][1], c = m.m[0][2],
	      d = m.m[1][0], e = m.m[1][1], f = m.m[1][2],
	      g = m.m[2][0], h = m.m[2][1], i = m.m[2][2];

	result.m[0][0] =   e * i - f * h;
	result.m[0][1] = -(b * i - h * c);
	result.m[0][2] =   b * f - e * c;
	result.m[1][0] = -(d * i - g * f);
	result.m[1][1] =   a * i - c * g;
	result.m[1][2] = -(a * f - d * c);
	result.m[2][0] =   d * h - g * e;
	result.m[2][1] = -(a * h - g * b);
	result.m[2][2] =   a * e - b * d;

	det = 1.0f / (a * result.m[0][0] + b * result.m[1][0] + c * result.m[2][0]);

	result.m[0][0] *= det;
	result.m[0][1] *= det;
	result.m[0][2] *= det;
	result.m[1][0] *= det;
	result.m[1][1] *= det;
	result.m[1][2] *= det;
	result.m[2][0] *= det;
	result.m[2][1] *= det;
	result.m[2][2] *= det;

	return result;
}

inline mat4 inverse(const mat4& mat)
{
	//TODO: this function is not SIMD optimized, figure out how to do it

	mat4 result;

	float tmp[6];
	float det;
	float a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2], d = mat.m[0][3],
	      e = mat.m[1][0], f = mat.m[1][1], g = mat.m[1][2], h = mat.m[1][3],
	      i = mat.m[2][0], j = mat.m[2][1], k = mat.m[2][2], l = mat.m[2][3],
	      m = mat.m[3][0], n = mat.m[3][1], o = mat.m[3][2], p = mat.m[3][3];

	tmp[0] = k * p - o * l; 
	tmp[1] = j * p - n * l; 
	tmp[2] = j * o - n * k;
	tmp[3] = i * p - m * l; 
	tmp[4] = i * o - m * k; 
	tmp[5] = i * n - m * j;

	result.m[0][0] =   f * tmp[0] - g * tmp[1] + h * tmp[2];
	result.m[1][0] = -(e * tmp[0] - g * tmp[3] + h * tmp[4]);
	result.m[2][0] =   e * tmp[1] - f * tmp[3] + h * tmp[5];
	result.m[3][0] = -(e * tmp[2] - f * tmp[4] + g * tmp[5]);

	result.m[0][1] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
	result.m[1][1] =   a * tmp[0] - c * tmp[3] + d * tmp[4];
	result.m[2][1] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
	result.m[3][1] =   a * tmp[2] - b * tmp[4] + c * tmp[5];

	tmp[0] = g * p - o * h;
	tmp[1] = f * p - n * h;
	tmp[2] = f * o - n * g;
	tmp[3] = e * p - m * h;
	tmp[4] = e * o - m * g;
	tmp[5] = e * n - m * f;

	result.m[0][2] =   b * tmp[0] - c * tmp[1] + d * tmp[2];
	result.m[1][2] = -(a * tmp[0] - c * tmp[3] + d * tmp[4]);
	result.m[2][2] =   a * tmp[1] - b * tmp[3] + d * tmp[5];
	result.m[3][2] = -(a * tmp[2] - b * tmp[4] + c * tmp[5]);

	tmp[0] = g * l - k * h;
	tmp[1] = f * l - j * h;
	tmp[2] = f * k - j * g;
	tmp[3] = e * l - i * h;
	tmp[4] = e * k - i * g;
	tmp[5] = e * j - i * f;

	result.m[0][3] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
	result.m[1][3] =   a * tmp[0] - c * tmp[3] + d * tmp[4];
	result.m[2][3] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
  	result.m[3][3] =   a * tmp[2] - b * tmp[4] + c * tmp[5];

  	det = 1.0f / (a * result.m[0][0] + b * result.m[1][0]
                + c * result.m[2][0] + d * result.m[3][0]);

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(det);
	result.packed[0] = _mm_mul_ps(result.packed[0], scale);
	result.packed[1] = _mm_mul_ps(result.packed[1], scale);
	result.packed[2] = _mm_mul_ps(result.packed[2], scale);
	result.packed[3] = _mm_mul_ps(result.packed[3], scale);

	#else

	result.m[0][0] = result.m[0][0] * det;
	result.m[0][1] = result.m[0][1] * det;
	result.m[0][2] = result.m[0][2] * det;
	result.m[0][3] = result.m[0][3] * det;
	result.m[1][0] = result.m[1][0] * det;
	result.m[1][1] = result.m[1][1] * det;
	result.m[1][2] = result.m[1][2] * det;
	result.m[1][3] = result.m[1][3] * det;
	result.m[2][0] = result.m[2][0] * det;
	result.m[2][1] = result.m[2][1] * det;
	result.m[2][2] = result.m[2][2] * det;
	result.m[2][3] = result.m[2][3] * det;
	result.m[3][0] = result.m[3][0] * det;
	result.m[3][1] = result.m[3][1] * det;
	result.m[3][2] = result.m[3][2] * det;
	result.m[3][3] = result.m[3][3] * det;

	#endif

  	return result;
}

//translation:

inline mat3 translate(const vec2& t)
{
	mat3 result = mat3_identity();

	result.m[2][0] = t.x;
	result.m[2][1] = t.y;

	return result;
}

inline mat4 translate(const vec3& t)
{
	mat4 result = mat4_identity();

	result.m[3][0] = t.x;
	result.m[3][1] = t.y;
	result.m[3][2] = t.z;

	return result;
}

//scaling:

inline mat3 scale(const vec2& s)
{
	mat3 result = mat3_identity();

	result.m[0][0] = s.x;
	result.m[1][1] = s.y;

	return result;
}

inline mat4 scale(const vec3& s)
{
	mat4 result = mat4_identity();

	result.m[0][0] = s.x;
	result.m[1][1] = s.y;
	result.m[2][2] = s.z;

	return result;
}

//rotation:

inline mat3 rotate(float angle)
{
	mat3 result = mat3_identity();

	float radians = deg_to_rad(angle);
	float sine   = QM_SINF(radians);
	float cosine = QM_COSF(radians);

	result.m[0][0] = cosine;
	result.m[1][0] =   sine;
	result.m[0][1] =  -sine;
	result.m[1][1] = cosine;

	return result;
}

inline mat4 rotate(const vec3& axis, float angle)
{
	mat4 result = mat4_identity();

	vec3 normalized = normalize(axis);

	float radians = deg_to_rad(angle);
	float sine    = QM_SINF(radians);
	float cosine  = QM_COSF(radians);
	float cosine2 = 1.0f - cosine;

	result.m[0][0] = normalized.x * normalized.x * cosine2 + cosine;
	result.m[0][1] = normalized.x * normalized.y * cosine2 + normalized.z * sine;
	result.m[0][2] = normalized.x * normalized.z * cosine2 - normalized.y * sine;
	result.m[1][0] = normalized.y * normalized.x * cosine2 - normalized.z * sine;
	result.m[1][1] = normalized.y * normalized.y * cosine2 + cosine;
	result.m[1][2] = normalized.y * normalized.z * cosine2 + normalized.x * sine;
	result.m[2][0] = normalized.z * normalized.x * cosine2 + normalized.y * sine;
	result.m[2][1] = normalized.z * normalized.y * cosine2 - normalized.x * sine;
	result.m[2][2] = normalized.z * normalized.z * cosine2 + cosine;

	return result;
}

inline mat4 rotate(const vec3& euler)
{
	mat4 result = mat4_identity();

	vec3 radians;
	radians.x = deg_to_rad(euler.x);
	radians.y = deg_to_rad(euler.y);
	radians.z = deg_to_rad(euler.z);

	float sinX = QM_SINF(radians.x);
	float cosX = QM_COSF(radians.x);
	float sinY = QM_SINF(radians.y);
	float cosY = QM_COSF(radians.y);
	float sinZ = QM_SINF(radians.z);
	float cosZ = QM_COSF(radians.z);

	result.m[0][0] = cosY * cosZ;
	result.m[0][1] = cosY * sinZ;
	result.m[0][2] = -sinY;
	result.m[1][0] = sinX * sinY * cosZ - cosX * sinZ;
	result.m[1][1] = sinX * sinY * sinZ + cosX * cosZ;
	result.m[1][2] = sinX * cosY;
	result.m[2][0] = cosX * sinY * cosZ + sinX * sinZ;
	result.m[2][1] = cosX * sinY * sinZ - sinX * cosZ;
	result.m[2][2] = cosX * cosY;

	return result;
}

//to mat3:

inline mat3 top_left(const mat4& m)
{
	mat3 result;

	result.m[0][0] = m.m[0][0];
	result.m[0][1] = m.m[0][1];
	result.m[0][2] = m.m[0][2];
	result.m[1][0] = m.m[1][0];
	result.m[1][1] = m.m[1][1];
	result.m[1][2] = m.m[1][2];
	result.m[2][0] = m.m[2][0];
	result.m[2][1] = m.m[2][1];
	result.m[2][2] = m.m[2][2];

	return result;
}

//projection:

inline mat4 perspective(float fov, float aspect, float near, float far)
{
	mat4 result;

	float scale = QM_TANF(deg_to_rad(fov * 0.5f)) * near;

	float right = aspect * scale;
	float left  = -right;
	float top   = scale;
	float bot   = -top;

	result.m[0][0] = near / right;
	result.m[1][1] = near / top;
	result.m[2][2] = -(far + near) / (far - near);
	result.m[3][2] = -2.0f * far * near / (far - near);
	result.m[2][3] = -1.0f;

	return result;
}

inline mat4 orthographic(float left, float right, float bot, float top, float near, float far)
{
	mat4 result = mat4_identity();

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bot);
	result.m[2][2] = 2.0f / (near - far);

	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (bot  + top  ) / (bot  - top  );
	result.m[3][2] = (near + far  ) / (near - far  );

	return result;
}

//view matrix:

inline mat4 look(const vec3& pos, const vec3& dir, const vec3& up)
{
	mat4 result;

	vec3 r = normalize(cross(up, dir));
	vec3 u = cross(dir, r);

	mat4 RUD = mat4_identity();
	RUD.m[0][0] = r.x;
	RUD.m[1][0] = r.y;
	RUD.m[2][0] = r.z;
	RUD.m[0][1] = u.x;
	RUD.m[1][1] = u.y;
	RUD.m[2][1] = u.z;
	RUD.m[0][2] = dir.x;
	RUD.m[1][2] = dir.y;
	RUD.m[2][2] = dir.z;

	vec3 oppPos = {-pos.x, -pos.y, -pos.z};	
	result = RUD * translate(oppPos);

	return result;
}

inline mat4 lookat(const vec3& pos, const vec3& target, const vec3& up)
{
	mat4 result;

	vec3 dir = normalize(pos - target);
	result = look(pos, dir, up);

	return result;
}

//----------------------------------------------------------------------//
//QUATERNION FUNCTIONS:

#if QM_INCLUDE_IOSTREAM

inline std::ostream& operator<<(std::ostream& os, const quaternion& q)
{
	os << q.x << ", " << q.y << ", " << q.z << ", " << q.w;
	return os;
}

inline std::istream& operator>>(std::istream& is, quaternion& q)
{
	is >> q.x >> q.y >> q.z >> q.w;
	return is;
}

#endif

inline quaternion quaternion_identity()
{
	quaternion result;

	result.x = 0.0f;
	result.y = 0.0f;
	result.z = 0.0f;
	result.w = 1.0f;

	return result;
}

inline quaternion operator+(const quaternion& q1, const quaternion& q2)
{
	quaternion result;

	#if QM_USE_SSE

	result.packed = _mm_add_ps(q1.packed, q2.packed);

	#else

	result.x = q1.x + q2.x;
	result.y = q1.y + q2.y;
	result.z = q1.z + q2.z;
	result.w = q1.w + q2.w;

	#endif

	return result;
}

inline quaternion operator-(const quaternion& q1, const quaternion& q2)
{
	quaternion result;

	#if QM_USE_SSE

	result.packed = _mm_sub_ps(q1.packed, q2.packed);

	#else

	result.x = q1.x - q2.x;
	result.y = q1.y - q2.y;
	result.z = q1.z - q2.z;
	result.w = q1.w - q2.w;

	#endif

	return result;
}

inline quaternion operator*(const quaternion& q1, const quaternion& q2)
{
	quaternion result;

	#if QM_USE_SSE

	__m128 temp1;
	__m128 temp2;

	temp1 = _mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(3, 3, 3, 3));
	temp2 = q2.packed;
	result.packed = _mm_mul_ps(temp1, temp2);

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(0, 1, 2, 3));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(1, 0, 3, 2));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f));
	temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(2, 3, 0, 1));
	result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

	#else

	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

	#endif

	return result;
}

inline quaternion operator*(const quaternion& q, float s)
{
	quaternion result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_mul_ps(q.packed, scale);

	#else

	result.x = q.x * s;
	result.y = q.y * s;
	result.z = q.z * s;
	result.w = q.w * s;

	#endif

	return result;
}

inline quaternion operator*(float s, const quaternion& q)
{
	return q * s;
}

inline quaternion operator/(const quaternion& q, float s)
{
	quaternion result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_div_ps(q.packed, scale);

	#else

	result.x = q.x / s;
	result.y = q.y / s;
	result.z = q.z / s;
	result.w = q.w / s;

	#endif

	return result;
}

inline quaternion operator/(float s, const quaternion& q)
{
	quaternion result;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(s);
	result.packed = _mm_div_ps(scale, q.packed);

	#else

	result.x = s / q.x;
	result.y = s / q.y;
	result.z = s / q.z;
	result.w = s / q.w;

	#endif

	return result;
}

inline float dot(const quaternion& q1, const quaternion& q2)
{
	float result;

	#if QM_USE_SSE

	__m128 r = _mm_mul_ps(q1.packed, q2.packed);
	r = _mm_hadd_ps(r, r);
	r = _mm_hadd_ps(r, r);
	_mm_store_ss(&result, r);

	#else

	result = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

	#endif

	return result;
}

inline float length(const quaternion& q)
{
	float result;

	result = QM_SQRTF(dot(q, q));

	return result;
}

inline quaternion normalize(const quaternion& q)
{
	quaternion result;

	float len = length(q);
	if(len != 0.0f)
		result = q / len;

	return result;
}

inline quaternion conjugate(const quaternion& q)
{
	quaternion result;

	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	result.w = q.w;

	return result;
}

inline quaternion inverse(const quaternion& q)
{
	quaternion result;

	result.x = -q.x;
	result.y = -q.y;
	result.z = -q.z;
	result.w = q.w;

	#if QM_USE_SSE

	__m128 scale = _mm_set1_ps(dot(q, q));
	_mm_div_ps(result.packed, scale);

	#else

	float invLen2 = 1.0f / QM_PREFIX(quaternion_dot)(q, q);

	result.x *= invLen2;
	result.y *= invLen2;
	result.z *= invLen2;
	result.w *= invLen2;

	#endif

	return result;
}

inline quaternion slerp(const quaternion& q1, const quaternion& q2, float a)
{
	quaternion result;

	float cosine = dot(q1, q2);
	float angle = QM_ACOSF(cosine);

	float sine1 = QM_SINF((1.0f - a) * angle);
	float sine2 = QM_SINF(a * angle);
	float invSine = 1.0f / QM_SINF(angle);

	quaternion q1scaled = q1 * sine1;
	quaternion q2scaled = q2 * sine2;

	result = q1scaled + q2scaled;
	result = result * invSine;

	return result;
}

inline bool operator==(const quaternion& q1, const quaternion& q2)
{
	bool result;

	//TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
	result = (q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w); 

	return result;	
}

inline bool operator!=(const quaternion& q1, const quaternion& q2)
{
	bool result;

	result = (q1.x != q2.x) || (q1.y != q2.y) || (q1.z != q2.z) || (q1.w != q2.w); 

	return result;
}

inline quaternion quaternion_from_axis_angle(const vec3& axis, float angle)
{
	quaternion result;

	float radians = deg_to_rad(angle * 0.5f);
	vec3 normalized = normalize(axis);
	float sine = QM_SINF(radians);

	result.x = normalized.x * sine;
	result.y = normalized.y * sine;
	result.z = normalized.z * sine;
	result.w = QM_COSF(radians);

	return result;
}

inline quaternion quaternion_from_euler(const vec3& angles)
{
	quaternion result;

	vec3 radians;
	radians.x = deg_to_rad(angles.x * 0.5f);
	radians.y = deg_to_rad(angles.y * 0.5f);
	radians.z = deg_to_rad(angles.z * 0.5f);

	float sinx = QM_SINF(radians.x);
	float cosx = QM_COSF(radians.x);
	float siny = QM_SINF(radians.y);
	float cosy = QM_COSF(radians.y);
	float sinz = QM_SINF(radians.z);
	float cosz = QM_COSF(radians.z);

	#if QM_USE_SSE

	__m128 packedx = _mm_setr_ps(sinx, cosx, cosx, cosx);
	__m128 packedy = _mm_setr_ps(cosy, siny, cosy, cosy);
	__m128 packedz = _mm_setr_ps(cosz, cosz, sinz, cosz);

	result.packed = _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz);

	packedx = _mm_shuffle_ps(packedx, packedx, _MM_SHUFFLE(0, 0, 0, 1));
	packedy = _mm_shuffle_ps(packedy, packedy, _MM_SHUFFLE(1, 1, 0, 1));
	packedz = _mm_shuffle_ps(packedz, packedz, _MM_SHUFFLE(2, 0, 2, 2));

	result.packed = _mm_addsub_ps(result.packed, _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz));

	#else

	result.x = sinx * cosy * cosz - cosx * siny * sinz;
	result.y = cosx * siny * cosz + sinx * cosy * sinz;
	result.z = cosx * cosy * sinz - sinx * siny * cosz;
	result.w = cosx * cosy * cosz + sinx * siny * sinz;

	#endif

	return result;
}

inline mat4 quaternion_to_mat4(const quaternion& q)
{
	mat4 result = mat4_identity();

	float x2  = q.x + q.x;
    float y2  = q.y + q.y;
    float z2  = q.z + q.z;
    float xx2 = q.x * x2;
    float xy2 = q.x * y2;
    float xz2 = q.x * z2;
    float yy2 = q.y * y2;
    float yz2 = q.y * z2;
    float zz2 = q.z * z2;
    float sx2 = q.w * x2;
    float sy2 = q.w * y2;
    float sz2 = q.w * z2;

	result.m[0][0] = 1.0f - (yy2 + zz2);
	result.m[0][1] = xy2 - sz2;
	result.m[0][2] = xz2 + sy2;
	result.m[1][0] = xy2 + sz2;
	result.m[1][1] = 1.0f - (xx2 + zz2);
	result.m[1][2] = yz2 - sx2;
	result.m[2][0] = xz2 - sy2;
	result.m[2][1] = yz2 + sx2;
	result.m[2][2] = 1.0f - (xx2 + yy2);

	return result;
}

}; //namespace qm

#endif //QM_MATH_H