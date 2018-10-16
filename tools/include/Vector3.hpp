#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <string>

class Vector3{
  public:

	double axis[3];

	Vector3(){ axis[0] = 0.0; axis[1] = 0.0; axis[2] = 0.0; }

	Vector3(const double &x, const double &y){ axis[0] = x; axis[1] = y; axis[2] = 0.0; }

	Vector3(const double &x, const double &y, const double &z){ axis[0] = x; axis[1] = y; axis[2] = z; }

	void Set(const double &x, const double &y, const double &z){ axis[0] = x; axis[1] = y; axis[2] = z; }

	void Get(double &x, double &y, double &z){ x = axis[0]; y = axis[1]; z = axis[2]; }

	const Vector3& operator = (const Vector3&);

	const Vector3& operator += (const Vector3&);

	const Vector3& operator -= (const Vector3&);

	const Vector3& operator *= (const double&);

	Vector3 operator + (const Vector3&) const ;

	Vector3 operator - (const Vector3&) const ;

	Vector3 operator * (const double&) const ;

	double Dot(const Vector3 &) const ;

	double CosAngle(const Vector3 &) const ;

	Vector3 Cross(const Vector3 &) const ;

	double Length() const ;

	double Square() const ; 

	double Distance(const Vector3 &) const ;

	double Normalize();

	std::string Dump() const ;
};

void Cart2Sphere(const double &x, const double &y, const double &z, double &r, double &theta, double &phi);

void Cart2Sphere(const double &x, const double &y, const double &z, Vector3 &sphere);

void Cart2Sphere(const Vector3 &cart, double &r, double &theta, double &phi);

void Cart2Sphere(const Vector3 &cart, Vector3 &sphere);

void Cart2Sphere(Vector3&);

void Sphere2Cart(const double &r, const double &theta, const double &phi, double &x, double &y, double &z);

void Sphere2Cart(const double &r, const double &theta, const double &phi, Vector3 &cart);

void Sphere2Cart(const Vector3 &sphere, double &x, double &y, double &z);

void Sphere2Cart(const Vector3 &sphere, Vector3 &cart);

void Sphere2Cart(Vector3&);

#endif
