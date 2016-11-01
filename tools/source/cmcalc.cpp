#include <iostream>
#include <sstream>
#include <string>
#include <cmath>

#include "cmcalc.hpp"

const double c = 2.99792458E8; // m/s
const double cSquare = 8.9875517873E16; // m^2/s^2
const double pi = 3.14159265359;
const double twoPi = 6.28318530718;

const double proton_RME = 938.272046; // MeV/c^2
const double neutron_RME = 939.565378; // MeV/c^2
const double mev2amu = 1.0/931.494061; // (amu*c^2)/MeV
const double mev2kg = 1.783E-30; // (kg*c^2)/MeV

double getMin(const double &v1_, const double &v2_){
	if(v1_ >= 0.0 && v2_ >= 0.0) return (v1_ < v2_) ? v1_ : v2_;
	else if(v1_ >= 0.0) return v1_;
	else if(v2_ >= 0.0) return v2_;
	else return -1;
}

double getMax(const double &v1_, const double &v2_){
	if(v1_ >= 0.0 && v2_ >= 0.0) return (v1_ > v2_) ? v1_ : v2_;
	else if(v1_ >= 0.0) return v1_;
	else if(v2_ >= 0.0) return v2_;
	else return -1;
}

double dabs(const double &in_){
	return (in_ < 0.0) ? -1*in_ : in_;
}

particle::particle(const double &thetaLab_, const double &Vcm_, const double &partVcm_, const double &mass_){
	calculate(thetaLab_, Vcm_, partVcm_, mass_);
}

bool particle::calculate(const double &thetaLab_, const double &Vcm_, const double &partVcm_, const double &mass_){
	labAngle[0] = thetaLab_;
	labAngle[1] = thetaLab_;

	if(partVcm_ < Vcm_ && (thetaLab_ > std::asin(partVcm_/Vcm_)*180/pi)){
		v[0] = -1; v[1] = -1;
		E[0] = -1; E[1] = -1;
		comAngle[0] = -1;
		comAngle[1] = -1;
	
		return false;
	}

	double sinTheta = std::sin(thetaLab_ * pi / 180);
	double cosTheta = std::cos(thetaLab_ * pi / 180);
	double tempArg = std::sqrt(partVcm_*partVcm_ - Vcm_*Vcm_*sinTheta*sinTheta);

	// Calculate the velocity for the first solution.
	v[0] = Vcm_ * cosTheta + tempArg;
	E[0] = 0.5 * mass_ * v[0]*v[0] / cSquare;
	comAngle[0] = std::atan2(sinTheta, cosTheta - Vcm_/v[0]) * 180 / pi;
	
	if(partVcm_ <= Vcm_){
		v[1] = Vcm_ * cosTheta - tempArg;
		E[1] = 0.5 * mass_ * v[1]*v[1] / cSquare;
		comAngle[1] = std::atan2(sinTheta, cosTheta - Vcm_/v[1]) * 180 / pi;
	}
	else{
		v[1] = -1;
		E[1] = -1;
		comAngle[1] = -1;
	}
	
	return true;
}

bool particle::calculateCOM(const double &thetaCOM_, const double &Vcm_, const double &partVcm_, const double &mass_){
	comAngle[0] = thetaCOM_;
	comAngle[1] = -1;

	double sinTheta = std::sin(thetaCOM_ * pi / 180);
	double cosTheta = std::cos(thetaCOM_ * pi / 180);
	double tempArg = 2*Vcm_*partVcm_*std::cos(thetaCOM_*pi/180);

	// Calculate velocity and energy solutions.
	v[0] = std::sqrt(Vcm_*Vcm_ + partVcm_*partVcm_ + tempArg);
	v[1] = -1;
	E[0] = 0.5 * mass_ * v[0]*v[0] / cSquare;
	E[1] = -1;

	// Calculate the lab angles for each solution.
	labAngle[0] = dabs(std::atan2(sinTheta, Vcm_/partVcm_ + cosTheta) * 180 / pi);
	labAngle[1] = -1;
	
	return true;
}

std::string particle::print(){
	std::stringstream stream;
	if(v[0] >= 0.0) stream << comAngle[0] << "\t" << labAngle[0] << "\t" << E[0] << "\t" << v[0];
	else stream << "undefined\tundefined\tundefined\tundefined";
	if(v[1] >= 0.0) stream << "\t" << comAngle[1] << "\t" << labAngle[1] << "\t" << E[1] << "\t" << v[1];
	else stream << "\tundefined\tundefined\tundefined\tundefined";
	return stream.str();
}

void reaction::Calculate(){
	// Must calculate these first. Other formulas rely on their values.
	Ecm = (Mtarg / (Mbeam + Mtarg))*Ebeam;
	Qgs = Mbeam + Mtarg - Meject - Mrecoil;
	Qfinal = (Qgs - EXrecoil - EXeject);
	
	// The following formulas rely on Ecm and Qgs.
	Vcm = (c / (Mbeam + Mtarg)) * std::sqrt(2 * Mbeam * Ebeam);
	recoilVcm = Vcm / std::sqrt((Mbeam / Mtarg) * (Mrecoil / Meject) * (Ecm / (Ecm + Qfinal)));
	ejectVcm = Vcm / std::sqrt((Mbeam / Mtarg) * (Meject / Mrecoil) * (Ecm / (Ecm + Qfinal)));
	
	recoilMaxAngle = recoilVcm > Vcm ? 180.0 : std::asin(recoilVcm/Vcm)*180/pi;
	ejectMaxAngle = ejectVcm > Vcm ? 180.0 : std::asin(ejectVcm/Vcm)*180/pi;
}

double reaction::SetBeam(const int &Z_, const int &A_, const double &BE_A_/*=0.0*/){
	return (Mbeam = Z_*proton_RME + (A_-Z_)*neutron_RME - BE_A_*A_);
}

double reaction::SetTarget(const int &Z_, const int &A_, const double &BE_A_/*=0.0*/){
	return (Mtarg = Z_*proton_RME + (A_-Z_)*neutron_RME - BE_A_*A_);
}

double reaction::SetRecoil(const int &Z_, const int &A_, const double &BE_A_/*=0.0*/){
	return (Mrecoil = Z_*proton_RME + (A_-Z_)*neutron_RME - BE_A_*A_);
}

double reaction::SetEjectile(const int &Z_, const int &A_, const double &BE_A_/*=0.0*/){
	return (Meject = Z_*proton_RME + (A_-Z_)*neutron_RME - BE_A_*A_);
}

double reaction::SetRecoilEx(const double &ex_){
	return (EXrecoil = ex_);
}

double reaction::SetEjectileEx(const double &ex_){
	return (EXeject = ex_);
}

double reaction::SetEbeam(const double &E_){
	Ebeam = E_;
	Calculate();
	return Ebeam;
}

void reaction::SetLabAngle(const double &thetaLab_){
	recoilPart.calculate(thetaLab_, Vcm, recoilVcm, Mrecoil);
	ejectPart.calculate(thetaLab_, Vcm, ejectVcm, Meject);
}

void reaction::SetComAngle(const double &thetaCom_){
	recoilPart.calculateCOM(thetaCom_, Vcm, recoilVcm, Mrecoil);
	ejectPart.calculateCOM(thetaCom_, Vcm, ejectVcm, Meject);
}

void reaction::Print(){
	particle tempParticle1;
	particle tempParticle2;

	std::cout << " Reaction Information:\n";
	std::cout << "  Beam Mass (MeV/c^2)       = " << Mbeam << std::endl;
	std::cout << "  Beam Energy (MeV)         = " << Ebeam << std::endl;
	std::cout << "  Beam Velocity (m/s)       = " << (c * std::sqrt(2 * Ebeam / Mbeam)) << std::endl;
	std::cout << "  Target Mass (MeV/c^2)     = " << Mtarg << std::endl;
	
	tempParticle1.calculateCOM(0, Vcm, recoilVcm, Mrecoil);
	tempParticle2.calculateCOM(180, Vcm, recoilVcm, Mrecoil);
	std::cout << "  Recoil Mass (MeV/c^2)     = " << Mrecoil << std::endl;
	std::cout << "  Recoil Excitation (MeV)   = " << EXrecoil << std::endl;
	std::cout << "  Recoil Max Angle (deg)    = " << recoilMaxAngle << std::endl;
	std::cout << "  Recoil Max Energy (MeV)   = " << getMax(getMax(tempParticle1.E[0], tempParticle1.E[1]), getMax(tempParticle2.E[0], tempParticle2.E[1])) << std::endl;
	std::cout << "  Recoil Min Energy (MeV)   = " << getMin(getMin(tempParticle1.E[0], tempParticle1.E[1]), getMin(tempParticle2.E[0], tempParticle2.E[1])) << std::endl;
	
	tempParticle1.calculateCOM(0, Vcm, ejectVcm, Meject);
	tempParticle2.calculateCOM(180, Vcm, ejectVcm, Meject);
	std::cout << "  Ejectile Mass (MeV/c^2)   = " << Meject << std::endl;
	std::cout << "  Ejectile Excitation (MeV) = " << EXeject << std::endl;
	std::cout << "  Ejectile Max Angle (deg)  = " << ejectMaxAngle << std::endl;
	std::cout << "  Ejectile Max Energy (MeV) = " << getMax(getMax(tempParticle1.E[0], tempParticle1.E[1]), getMax(tempParticle2.E[0], tempParticle2.E[1])) << std::endl;
	std::cout << "  Ejectile Min Energy (MeV) = " << getMin(getMin(tempParticle1.E[0], tempParticle1.E[1]), getMin(tempParticle2.E[0], tempParticle2.E[1])) << std::endl;
	
	std::cout << "\n  Threshold (MeV)            = " << (Ecm + Qgs) << std::endl;
	std::cout << "  Beam CM Velocity (m/s)     = " << Vcm << std::endl;
	std::cout << "  Recoil CM Velocity (m/s)   = " << recoilVcm << std::endl;
	std::cout << "  Ejectile CM Velocity (m/s) = " << ejectVcm << std::endl;
	std::cout << "  CM Energy (MeV)            = " << Ecm << std::endl;
	std::cout << "  G.S. Q-value (MeV)         = " << Qgs << std::endl;
	std::cout << "  Final Q-value (MeV)        = " << Qfinal << std::endl;
	
	if(Mtarg < Mbeam) std::cout << "\n  Inverse Kinematics\n";
	else std::cout << "\n  Normal Kinematics\n";
	
	if(Ecm + Qfinal > 0.0) std::cout << "  Above Threshold\n";
	else std::cout << "  Below Threshold\n";
}
