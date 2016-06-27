#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>

class calibration{
  public:
	int id;
	double b;
	double m;
	
	calibration() : id(0), b(0.0), m(1.0){ }
	
	calibration(const int &id_) : id(id_), b(0.0), m(1.0){ }
	
	calibration(const int &id_, const double &b_, const double &m_) : id(id_), b(b_), m(m_){ }
	
	double getEnergy(const double &chan_){ return m*chan_+b; }
	
	double getChan(const double &energy_){ return (energy_-b)/m; }
};

class CalibFile{
  private:
  	std::vector<calibration> calib;
	bool init;
	
  public:
	CalibFile() : init(false){ }
	
	CalibFile(const char *filename_);
	
	bool IsInit(){ return init; }
	
	bool Load(const char *filename_);
	
	double GetEnergy(const int &id_, const double &adc_);
};

#endif
