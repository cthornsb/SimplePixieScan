#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>

class XiaData;

class CalibEntry{
  public:
	unsigned int id;
	double r0;
	double theta;
	double phi;
	double t0;
	double slope;
	double offset;
	
	CalibEntry() : id(0), r0(0.0), theta(0.0), phi(0.0), t0(0.0), slope(0.0), offset(0.0) { }
	
	CalibEntry(const int &id_) : id(id_), r0(0.0), theta(0.0), phi(0.0), t0(0.0), slope(0.0), offset(0.0) { }
	
	CalibEntry(const int &id_, const double &r0_, const double &theta_, const double &phi_, const double &t0_, const double &slope_, const double &offset_) : 
		id(id_), r0(r0_), theta(theta_), phi(phi_), t0(t0_), slope(slope_), offset(offset_) { }
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
