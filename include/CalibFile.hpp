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
	double toffset;
	
	CalibEntry() : id(0), r0(0.0), theta(0.0), phi(0.0), toffset(0.0) { }
	
	CalibEntry(const int &id_) : id(id_), r0(0.0), theta(0.0), phi(0.0), toffset(0.0) { }
	
	CalibEntry(const int &id_, const double &r0_, const double &theta_, const double &phi_, const double &toffset_) : 
		id(id_), r0(r0_), theta(theta_), phi(phi_), toffset(toffset_) { }
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
