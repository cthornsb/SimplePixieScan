#ifndef MATRIX3_HPP
#define MATRIX3_HPP

#include <string>

#include "Vector3.hpp"

class Matrix3{
  public:

	double components[3][3];

	Matrix3();

	Matrix3(const double &theta_, const double &phi_, const double &psi_=0.0);

	Matrix3(const Vector3 &vector_);

	Matrix3(const double &a00, const double &a10, const double &a20,
	        const double &a01, const double &a11, const double &a21,
	        const double &a02, const double &a12, const double &a22);

	Matrix3 operator * (const Matrix3 &right);

	Matrix3 operator *= (const Matrix3 &right);	        

	void GetUnitX(Vector3 &vector_);

	void GetUnitY(Vector3 &vector_);

	void GetUnitZ(Vector3 &vector_);

	Vector3 GetUnitX();

	Vector3 GetUnitY();

	Vector3 GetUnitZ();

	void SetUnitX(const Vector3 &vector_);

	void SetUnitY(const Vector3 &vector_);

	void SetUnitZ(const Vector3 &vector_);

	void SetRow1(const double &p1, const double &p2, const double &p3);

	void SetRow2(const double &p1, const double &p2, const double &p3);

	void SetRow3(const double &p1, const double &p2, const double &p3);

	void SetRotationMatrix(const double &theta_, const double &phi_, const double &psi_=0.0);

	void SetRotationMatrix(const Vector3 &vector_);

	void Transform(Vector3 &vector_);

	void Transpose(Vector3 &vector_);

	std::string Dump();
	
  private:

	void _initialize();
};

#endif
