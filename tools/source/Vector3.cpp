#include <cmath>
#include <sstream>

#include "Vector3.hpp"

const double pi=3.14159265359;

const Vector3& Vector3::operator = (const Vector3 &other){
	axis[0] = other.axis[0];
	axis[1] = other.axis[1];
	axis[2] = other.axis[2];
	return *this;
}

// Vector addition
const Vector3& Vector3::operator += (const Vector3 &other){
	axis[0] += other.axis[0];
	axis[1] += other.axis[1];
	axis[2] += other.axis[2];
	return *this;
}

// Vector subtraction
const Vector3& Vector3::operator -= (const Vector3 &other){
	axis[0] -= other.axis[0];
	axis[1] -= other.axis[1];
	axis[2] -= other.axis[2];
	return *this;
}

// Scalar multiplication
const Vector3& Vector3::operator *= (const double &scalar){
	axis[0] *= scalar;
	axis[1] *= scalar;
	axis[2] *= scalar;
	return *this;
}

Vector3 Vector3::operator + (const Vector3 &other) const {
	return Vector3(axis[0]+other.axis[0], axis[1]+other.axis[1], axis[2]+other.axis[2]);
}

Vector3 Vector3::operator - (const Vector3 &other) const {
	return Vector3(axis[0]-other.axis[0], axis[1]-other.axis[1], axis[2]-other.axis[2]);
}

Vector3 Vector3::operator * (const double &scalar) const {
	return Vector3(axis[0]*scalar, axis[1]*scalar, axis[2]*scalar);
}

// Dot product
double Vector3::Dot(const Vector3 &other) const {
	return (axis[0]*other.axis[0] + axis[1]*other.axis[1] + axis[2]*other.axis[2]);
}

// Return the cosine of the angle between two vectors.
double Vector3::CosAngle(const Vector3 &other) const {
	return (this->Dot(other)/(this->Length()*other.Length()));
}

// Cross product
Vector3 Vector3::Cross(const Vector3 &other) const {
	return Vector3((axis[1]*other.axis[2]-other.axis[1]*axis[2]),
		       (other.axis[0]*axis[2]-axis[0]*other.axis[2]),
		       (axis[0]*other.axis[1]-other.axis[0]*axis[1]));
}

// Return the length of the vector
double Vector3::Length() const {
	return std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
}

// Return the square of the vector
double Vector3::Square() const {
	return axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2];
}

double Vector3::Distance(const Vector3 &other) const {
	double x = axis[0]-other.axis[0];
	double y = axis[1]-other.axis[1];
	double z = axis[2]-other.axis[2];
	return std::sqrt(x*x+y*y+z*z);
}

// Normalize the vector and return the normalization parameter
double Vector3::Normalize(){
	double parameter = Length();
	axis[0] = axis[0]/parameter;
	axis[1] = axis[1]/parameter;
	axis[2] = axis[2]/parameter;
	return parameter;
}

std::string Vector3::Dump() const { 
	std::stringstream stream;
	stream << axis[0] << ", " << axis[1] << ", " << axis[2]; 
	return stream.str();
}

/////////////////////////////////////////////////////////////////////
// Cart2Sphere
/////////////////////////////////////////////////////////////////////

void Cart2Sphere(const double &x, const double &y, const double &z, double &r, double &theta, double &phi){ 
	r = std::sqrt(x*x + y*y + z*z);
	theta = std::acos(z/r);
	
	if(x == 0.0 && y == 0.0){ phi = 0.0; }
	else{ 
		double temp = std::sqrt(x*x + y*y); 
		if(x >= 0.0){ phi = std::acos(y/temp); }
		else{ phi = 2.0*pi - std::acos(y/temp); }
	}
} 

void Cart2Sphere(const double &x, const double &y, const double &z, Vector3 &sphere){
	sphere.axis[0] = std::sqrt(x*x + y*y + z*z);
	sphere.axis[1] = std::acos(z/sphere.axis[0]);
	
	if(x == 0.0 && y == 0.0){ sphere.axis[2] = 0.0; }
	else{ 
		double temp = std::sqrt(x*x + y*y); 
		if(x >= 0.0){ sphere.axis[2] = std::acos(y/temp); }
		else{ sphere.axis[2] = 2.0*pi - std::acos(y/temp); }
	}
}

void Cart2Sphere(const Vector3 &cart, double &r, double &theta, double &phi){
	r = std::sqrt(cart.axis[0]*cart.axis[0] + cart.axis[1]*cart.axis[1] + cart.axis[2]*cart.axis[2]);
	theta = std::acos(cart.axis[2]/r);
	
	if(cart.axis[0] == 0.0 && cart.axis[1] == 0.0){ phi = 0.0; }
	else{ 
		double temp = std::sqrt(cart.axis[0]*cart.axis[0] + cart.axis[1]*cart.axis[1]); 
		if(cart.axis[0] >= 0.0){ phi = std::acos(cart.axis[1]/temp); }
		else{ phi = 2.0*pi - std::acos(cart.axis[1]/temp); }
	}
}

void Cart2Sphere(const Vector3 &cart, Vector3 &sphere){
	sphere.axis[0] = std::sqrt(cart.axis[0]*cart.axis[0] + cart.axis[1]*cart.axis[1] + cart.axis[2]*cart.axis[2]);
	sphere.axis[1] = std::acos(cart.axis[2]/sphere.axis[0]);
	
	if(cart.axis[0] == 0.0 && cart.axis[1] == 0.0){ sphere.axis[2] = 0.0; }
	else{ 
		double temp = std::sqrt(cart.axis[0]*cart.axis[0] + cart.axis[1]*cart.axis[1]); 
		if(cart.axis[0] >= 0.0){ sphere.axis[2] = std::acos(cart.axis[1]/temp); }
		else{ sphere.axis[2] = 2.0*pi - std::acos(cart.axis[1]/temp); }
	}
}

void Cart2Sphere(Vector3 &sphere){
	Vector3 cart = sphere;
	Cart2Sphere(cart, sphere);
}

/////////////////////////////////////////////////////////////////////
// Sphere2Cart
/////////////////////////////////////////////////////////////////////

void Sphere2Cart(const double &r, const double &theta, const double &phi, double &x, double &y, double &z){ 
	x = r*std::sin(theta)*std::cos(phi); 
	y = r*std::sin(theta)*std::sin(phi); 
	z = r*std::cos(theta);
}

void Sphere2Cart(const double &r, const double &theta, const double &phi, Vector3 &cart){
	cart.axis[0] = r*std::sin(theta)*std::cos(phi);
	cart.axis[1] = r*std::sin(theta)*std::sin(phi); 
	cart.axis[2] = r*std::cos(theta);
}

void Sphere2Cart(const Vector3 &sphere, double &x, double &y, double &z){
	x = sphere.axis[0]*std::sin(sphere.axis[1])*std::cos(sphere.axis[2]);
	y = sphere.axis[0]*std::sin(sphere.axis[1])*std::sin(sphere.axis[2]); 
	z = sphere.axis[0]*std::cos(sphere.axis[1]);
}

void Sphere2Cart(const Vector3 &sphere, Vector3 &cart){
	cart.axis[0] = sphere.axis[0]*std::sin(sphere.axis[1])*std::cos(sphere.axis[2]);
	cart.axis[1] = sphere.axis[0]*std::sin(sphere.axis[1])*std::sin(sphere.axis[2]); 
	cart.axis[2] = sphere.axis[0]*std::cos(sphere.axis[1]);
}

void Sphere2Cart(Vector3 &cart){
	Vector3 sphere = cart;
	Sphere2Cart(sphere, cart);
}
