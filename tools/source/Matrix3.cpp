#include <cmath>
#include <iomanip>
#include <sstream>

#include "Matrix3.hpp"

void Matrix3::_initialize(){
	for(unsigned int i = 0; i < 3; i++){ 
		components[i][0] = 0.0; 
		components[i][1] = 0.0; 
		components[i][2] = 0.0; 
	}
}

Matrix3::Matrix3(){ _initialize(); }

Matrix3::Matrix3(const double &theta_, const double &phi_, const double &psi_/*=0.0*/){
	_initialize();
	SetRotationMatrix(theta_, phi_, psi_);
}

Matrix3::Matrix3(const Vector3 &vector_){
	_initialize();
	SetRotationMatrix(vector_);
}

Matrix3::Matrix3(const double &a00, const double &a10, const double &a20,
	         const double &a01, const double &a11, const double &a21,
	         const double &a02, const double &a12, const double &a22){
	components[0][0] = a00; components[1][0] = a10; components[2][0] = a20; 
	components[0][1] = a01; components[1][1] = a11; components[2][1] = a21; 
	components[0][2] = a02; components[1][2] = a12; components[2][2] = a22; 
}

// Performs the operation (*this) x right
Matrix3 Matrix3::operator * (const Matrix3 &right){
	Matrix3 newMatrix;
	for(unsigned int i = 0; i < 3; i++){ // Over columns
		for(unsigned int j = 0; j < 3; j++){ // Over rows
			newMatrix.components[i][j] = 0;
			for(unsigned int k = 0; k < 3; k++){
				newMatrix.components[i][j] += components[i][k]*right.components[k][j];
			}
		}
	}
	return newMatrix;
}

// Performs the operation (*this) = (*this) x right
Matrix3 Matrix3::operator *= (const Matrix3 &right){
	Matrix3 oldMatrix(*this);
	for(unsigned int i = 0; i < 3; i++){ // Over columns
		for(unsigned int j = 0; j < 3; j++){ // Over rows
			components[i][j] = 0;
			for(unsigned int k = 0; k < 3; k++){
				components[i][j] += oldMatrix.components[i][k]*right.components[k][j];
			}
		}
	}
	return (*this);
}

void Matrix3::GetUnitX(Vector3 &vector_){ 
	vector_.axis[0] = components[0][0];  vector_.axis[1] = components[1][0];  vector_.axis[2] = components[2][0]; 
}

void Matrix3::GetUnitY(Vector3 &vector_){ 
	vector_.axis[0] = components[0][1];  vector_.axis[1] = components[1][1];  vector_.axis[2] = components[2][1]; 
}

void Matrix3::GetUnitZ(Vector3 &vector_){ 
	vector_.axis[0] = components[0][2];  vector_.axis[1] = components[1][2];  vector_.axis[2] = components[2][2]; 
}

Vector3 Matrix3::GetUnitX(){ 
	return Vector3(components[0][0], components[1][0], components[2][0]); 
}

Vector3 Matrix3::GetUnitY(){ 
	return Vector3(components[0][1], components[1][1], components[2][1]); 
}

Vector3 Matrix3::GetUnitZ(){ 
	return Vector3(components[0][2], components[1][2], components[2][2]); 
}

void Matrix3::SetUnitX(const Vector3 &vector_){ 
	components[0][0] = vector_.axis[0]; components[1][0] = vector_.axis[1]; components[2][0] = vector_.axis[2]; 
}

void Matrix3::SetUnitY(const Vector3 &vector_){ 
	components[0][1] = vector_.axis[0]; components[1][1] = vector_.axis[1]; components[2][1] = vector_.axis[2]; 
}

void Matrix3::SetUnitZ(const Vector3 &vector_){ 
	components[0][2] = vector_.axis[0]; components[1][2] = vector_.axis[1]; components[2][2] = vector_.axis[2]; 
}

void Matrix3::SetRow1(const double &p1, const double &p2, const double &p3){ 
	components[0][0] = p1; components[0][1] = p2; components[0][2] = p3; 
}

void Matrix3::SetRow2(const double &p1, const double &p2, const double &p3){ 
	components[1][0] = p1; components[1][1] = p2; components[1][2] = p3; 
}

void Matrix3::SetRow3(const double &p1, const double &p2, const double &p3){ 
	components[2][0] = p1; components[2][1] = p2; components[2][2] = p3; 
}

void Matrix3::SetRotationMatrix(const double &theta_, const double &phi_, const double &psi_/*=0.0*/){
	double sin_theta = std::sin(theta_), cos_theta = std::cos(theta_);
	double sin_phi = std::sin(phi_), cos_phi = std::cos(phi_);
	double sin_psi = std::sin(psi_), cos_psi = std::cos(psi_);
	
	// Pitch-Roll-Yaw convention
	// Rotate by angle theta about the y-axis
	//  angle phi about the z-axis
	//  angle psi about the x-axis	
	Matrix3 thetaM(cos_theta,         0,-sin_theta,
	                       0,         1,         0,
	               sin_theta,         0, cos_theta);
	Matrix3   phiM(  cos_phi,   sin_phi,         0,
	                -sin_phi,   cos_phi,         0,
	                       0,         0,         1);
	Matrix3   psiM(        1,         0,         0,
	                       0,   cos_psi,   sin_psi,
	                       0,  -sin_psi,   cos_psi);
	
	(*this) = psiM*(phiM*thetaM);
}

void Matrix3::SetRotationMatrix(const Vector3 &vector_){
	SetRotationMatrix(vector_.axis[1], vector_.axis[2]);
}

// Transform an input vector by this matrix
// Note: Expects the input vector to be in cartesian coordinates
void Matrix3::Transform(Vector3 &vector){
	double x = vector.axis[0], y = vector.axis[1], z = vector.axis[2];
	vector.axis[0] = components[0][0]*x + components[0][1]*y + components[0][2]*z;
	vector.axis[1] = components[1][0]*x + components[1][1]*y + components[1][2]*z;
	vector.axis[2] = components[2][0]*x + components[2][1]*y + components[2][2]*z;
}

// Transform an input vector by the transpose of this matrix
// Note: Expects the input vector to be in cartesian coordinates
void Matrix3::Transpose(Vector3 &vector){
	double x = vector.axis[0], y = vector.axis[1], z = vector.axis[2];
	vector.axis[0] = components[0][0]*x + components[1][0]*y + components[2][0]*z;
	vector.axis[1] = components[0][1]*x + components[1][1]*y + components[2][1]*z;
	vector.axis[2] = components[0][2]*x + components[1][2]*y + components[2][2]*z;
}

std::string Matrix3::Dump(){
	std::stringstream stream;
	stream.precision(3);
	stream << std::fixed;
	stream << "[" << std::setw(8) << components[0][0] << ", " << std::setw(8) << components[0][1] << ", " << std::setw(8) << components[0][2] << "]\n";
	stream << "[" << std::setw(8) << components[1][0] << ", " << std::setw(8) << components[1][1] << ", " << std::setw(8) << components[1][2] << "]\n";
	stream << "[" << std::setw(8) << components[2][0] << ", " << std::setw(8) << components[2][1] << ", " << std::setw(8) << components[2][2] << "]\n";
	return stream.str();
}
