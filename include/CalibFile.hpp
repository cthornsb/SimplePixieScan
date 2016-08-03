#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>
#include <string>

class XiaData;

class CalibEntry{
  public:
	unsigned int id;
	
	double r0;
	double theta;
	double phi;
	double t0;
	
	CalibEntry() : id(0), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { }
	
	CalibEntry(const int &id_) : id(id_), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { }
	
	CalibEntry(const int &id_, const double &r0_, const double &theta_, const double &phi_, const double &t0_) : 
		id(id_), r0(r0_), theta(theta_), phi(phi_), t0(t0_) { }

	CalibEntry(const std::vector<std::string> &pars_);
	
	bool Empty(){ return vals.empty(); }
	
	bool GetCalEnergy(const double &adc_, double &E);
	
	double GetCalEnergy(const double &adc_);
	
  private:
	std::vector<double> vals;
};

class CalibFile{
  private:
  	std::vector<CalibEntry> calib;
	bool init;
	
  public:
	CalibFile() : init(false) { }
	
	CalibFile(const char *filename_);
	
	bool IsInit(){ return init; }
	
	bool Load(const char *filename_);

	CalibEntry *GetCalibEntry(const unsigned int &id_);
	
	CalibEntry *GetCalibEntry(XiaData *event_);
};

#endif
