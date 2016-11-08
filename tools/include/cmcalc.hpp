#ifndef CMCALC_HPP
#define CMCALC_HPP

extern const double c, cSquare, pi, twoPi;
extern const double proton_RME, neutron_RME;
extern const double mev2amu, mev2kg;

double getMin(const double &v1_, const double &v2_);

double getMax(const double &v1_, const double &v2_);

double dabs(const double &in_);

class particle{
  public:
	double v[2];
	double E[2];
	double labAngle[2];
	double comAngle[2];
	
	particle(){ }
	
	particle(const double &thetaLab_, const double &Vcm_, const double &partVcm_, const double &mass_);
	
	bool calculate(const double &thetaLab_, const double &Vcm_, const double &partVcm_, const double &mass_);

	bool calculateCOM(const double &thetaCOM_, const double &Vcm_, const double &partVcm_, const double &mass_);
	
	std::string print();
};

class reaction{
  private:
	double Mbeam, Mtarg, Mrecoil, Meject;
	double Vcm, recoilVcm, ejectVcm;
	double recoilDelta, ejectDelta;
	double EXeject, EXrecoil;
	double Ebeam;
	double Ecm, Qgs;
	
	double recoilMaxAngle;
	double ejectMaxAngle;
	double Qfinal;
	
	particle recoilPart;
	particle ejectPart;
	
	void Calculate();
	
  public:
	reaction() : Mbeam(0.0), Mtarg(0.0), Mrecoil(0.0), Meject(0.0), 
	             Vcm(0.0), recoilVcm(0.0), ejectVcm(0.0), 
	             recoilDelta(0.0), ejectDelta(0.0), 
	             EXeject(0.0), EXrecoil(0.0), 
	             Ebeam(0.0), Ecm(0.0), Qgs(0.0) {}

	particle *GetRecoil(){ return &recoilPart; }
	
	particle *GetEjectile(){ return &ejectPart; }

	double GetBeamMass(){ return Mbeam; }
	
	double GetTargetMass(){ return Mtarg; }
	
	double GetRecoilMass(){ return Mrecoil; }
	
	double GetEjectileMass(){ return Meject; }
	
	double GetRecoilExcitation(){ return EXrecoil; }
	
	double GetEjectileExcitation(){ return EXeject; }
	
	double GetBeamEnergy(){ return Ebeam; }
	
	double GetComEnergy(){ return Ecm; }
	
	double GetQgs(){ return Qgs; }
	
	double GetQfinal(){ return Qfinal; }
	
	double GetRecoilMaxAngle(){ return recoilMaxAngle; }
	
	double GetEjectileMaxAngle(){ return ejectMaxAngle; }
	
	double GetThresholdEnergy(){ return (-Qfinal*(Mbeam+Mtarg)/Mtarg); }
	
	bool IsAboveThreshold(){ return ((Ecm + Qfinal >= 0.0) ? true : false); }

	bool IsNormalKinematics(){ return ((Mtarg >= Mbeam ) ? true : false); }

	double SetBeam(const int &Z_, const int &A_, const double &BE_A_=0.0);

	double SetTarget(const int &Z_, const int &A_, const double &BE_A_=0.0);

	double SetRecoil(const int &Z_, const int &A_, const double &BE_A_=0.0);

	double SetEjectile(const int &Z_, const int &A_, const double &BE_A_=0.0);

	double SetRecoilEx(const double &ex_);

	double SetEjectileEx(const double &ex_);

	double SetEbeam(const double &E_);

	void SetLabAngle(const double &thetaLab_);
	
	void SetComAngle(const double &thetaCom_);
	
	void Print();
};

#endif
