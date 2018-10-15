#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <string>

class Vector3{
  public:

	double axis[3];

	Vector3(){ axis[0] = 0.0; axis[1] = 0.0; axis[2] = 0.0; }

	Vector3(double x, double y){ axis[0] = x; axis[1] = y; axis[2] = 0.0; }

	Vector3(double x, double y, double z){ axis[0] = x; axis[1] = y; axis[2] = z; }

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

void Cart2Sphere(double, double, double, double&, double&, double&);

void Cart2Sphere(double, double, double, Vector3&);

void Cart2Sphere(const Vector3&, double&, double&, double&);

void Cart2Sphere(const Vector3&, Vector3&);

void Cart2Sphere(Vector3&);

void Sphere2Cart(double, double, double, double&, double&, double&);

void Sphere2Cart(double, double, double, Vector3&);

void Sphere2Cart(const Vector3&, double&, double&, double&);

void Sphere2Cart(const Vector3&, Vector3&);

void Sphere2Cart(Vector3&);

#endif
