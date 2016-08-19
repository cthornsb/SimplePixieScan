#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>
#include <string>

class XiaData;

class TimeCal{
  public:
	unsigned int id;
	
	double r0;
	double theta;
	double phi;
	double t0;
	
	TimeCal() : id(0), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { }
	
	TimeCal(const int &id_) : id(id_), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { }
	
	TimeCal(const int &id_, const double &r0_, const double &theta_, const double &phi_, const double &t0_) : 
		id(id_), r0(r0_), theta(theta_), phi(phi_), t0(t0_) { }
		
	TimeCal(const std::vector<std::string> &pars_);
	
	void Print();
};

class EnergyCal{
  public:
	unsigned int id;
	
	EnergyCal() : id(0) { }
	
	EnergyCal(const int &id_) : id(id_) { }
	
	EnergyCal(const std::vector<std::string> &pars_);
	
	bool Empty(){ return vals.empty(); }
	
	bool GetCalEnergy(const double &adc_, double &E);
	
	double GetCalEnergy(const double &adc_);

	void Print();

  private:
	std::vector<double> vals;
};

class CalibEntry{
  public:
	TimeCal *timeCal;
	EnergyCal *energyCal;
	
	CalibEntry() : timeCal(NULL), energyCal(NULL) { }
	
	CalibEntry(TimeCal *time_, EnergyCal *energy_) : timeCal(time_), energyCal(energy_) { }
	
	bool Time(){ return (timeCal != NULL); }
	
	bool Energy(){ return (energyCal != NULL); }
};

class CalibFile{
  private:
  	std::vector<TimeCal> time_calib;
  	std::vector<EnergyCal> energy_calib;

	bool _load(const char *filename_, const bool &time_);
	
  public:
	CalibFile(){ }
	
	CalibFile(const char *timeFilename_, const char *energyFilename_);
	
	bool LoadTimeCal(const char *filename_);

	bool LoadEnergyCal(const char *filename_);
	
	bool Load(const char *timeFilename_, const char *energyFilename_);

	TimeCal *GetTimeCal(const unsigned int &id_);
	
	TimeCal *GetTimeCal(XiaData *event_);

	EnergyCal *GetEnergyCal(const unsigned int &id_);
	
	EnergyCal *GetEnergyCal(XiaData *event_);
	
	CalibEntry *GetCalibEntry(const unsigned int &id_);
	
	CalibEntry *GetCalibEntry(XiaData *event_);
	
	void Debug(int mode);
};

extern CalibEntry dummyCalib;

#endif
