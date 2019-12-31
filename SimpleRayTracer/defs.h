#ifndef _DEFS_H_
#define _DEFS_H_

#include <cmath>
#include <iostream>

class Scene;
class Material;

/* 3 dimensional vector holding floating point numbers.
Used for both coordinates and color. 
*/
typedef struct Vector3f
{
	union {
		float x;
		float r;
	};
	union {
		float y;
		float g;
	};
	union {
		float z;
		float b;
	};

	float Length() { return std::sqrt(x * x + y * y + z * z); }

	Vector3f operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }
	Vector3f operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
	Vector3f operator+(float scalar) const { return {x + scalar, y + scalar, z + scalar}; }
	Vector3f operator-(float scalar) const { return {x - scalar, y - scalar, z - scalar}; }

	Vector3f operator/(const Vector3f& v) const { return { x / v.x, y / v.y, z / v.z }; }
	Vector3f operator*(const Vector3f& v) const { return {x * v.x, y * v.y, z * v.z}; }
	Vector3f operator+(const Vector3f& v) const { return {x + v.x, y + v.y, z + v.z}; }
	Vector3f operator-(const Vector3f& v) const { return {x - v.x, y - v.y, z - v.z}; }


	float Dot(const Vector3f& v) const { return x * v.x + y * v.y + z * v.z; }

	Vector3f Cross(const Vector3f& v) const
	{
		return {y * v.z - z * v.y,
				z * v.x - x * v.z,
				x * v.y - y * v.x };
	}

	Vector3f Normalize() {  float l = this->Length(); return  {x / l, y / l, z / l}; }

	void Print() const { std::cout << x << " " << y << " " << z << "\n"; }
	
} Vector3f;


/* Structure to hold return value from ray intersection routine. */
typedef struct ReturnVal
{
	Vector3f ip;
	Vector3f sn;
	Material* mat;

	ReturnVal(Vector3f p, Vector3f n, Material* m) : ip(p), sn(n), mat(m) { }
} ReturnVal;



//
// The global variable through which we can access Scene data
//
extern Scene* pScene;

#endif
