#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>
#include <string>

#include "CTerminal.h"
#include "ColorTerm.hpp"

#include "Matrix3.hpp"

extern const double deg2rad;
extern const double rad2deg;

class XiaData;
class TFile;

class CalType{
  public:
	unsigned int id;
	
	bool defaultVals;

	CalType() : id(0), defaultVals(true) { }
	
	CalType(const int &id_) : id(id_), defaultVals(true) { }
	
	virtual std::string Print(bool fancy=true){ return ""; }

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_) { return 0; }
};

class PositionCal : public CalType {
  public:
	double r0;
	double theta;
	double phi;
	double rotTheta;
	double rotPhi;
	double rotPsi;
	
	Vector3 position; // Position vector in cartesian coordinates.
	Matrix3 rotMatrix;

	PositionCal() : CalType(0), r0(0.0), theta(0.0), phi(0.0), rotTheta(0.0), rotPhi(0.0), rotPsi(0.0) { }
	
	PositionCal(const int &id_) : CalType(id_), r0(0.0), theta(0.0), phi(0.0), rotTheta(0.0), rotPhi(0.0), rotPsi(0.0) { }
	
	PositionCal(const int &id_, const double &r0_, const double &theta_, const double &phi_, const double &rotTheta_, const double &rotPhi_, const double &rotPsi_) : 
		CalType(id_), r0(r0_), theta(theta_), phi(phi_), rotTheta(rotTheta_), rotPhi(rotPhi_), rotPsi(rotPsi_) { }
		
	PositionCal(const std::vector<std::string> &pars_);

	Vector3 *GetPosition(){ return &position; }
	
	Matrix3 *GetRotationMatrix(){ return &rotMatrix; }
	
	void Transform(Vector3 &vec);

	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);
};

class TimeCal : public CalType {
  public:
	double t0;
	
	TimeCal() : CalType(0), t0(0.0) { }
	
	TimeCal(const int &id_) : CalType(id_), t0(0.0) { }
	
	TimeCal(const int &id_, const double &t0_) : 
		CalType(id_), t0(t0_) { }
		
	TimeCal(const std::vector<std::string> &pars_);

	bool GetCalTime(double &T);
	
	double GetCalTime(const double &time_);
	
	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);
};

class EnergyCal : public CalType {
  public:
	EnergyCal() : CalType(0) { }
	
	EnergyCal(const int &id_) : CalType(id_) { }
	
	EnergyCal(const std::vector<std::string> &pars_);
	
	bool Empty(){ return vals.empty(); }
	
	bool GetCalEnergy(const double &adc_, double &E);
	
	double GetCalEnergy(const double &adc_);

	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);

  private:
	std::vector<double> vals;
};

class BarCal : public CalType {
  public:
	double t0;
	double beta;
	double cbar;
	double length;
	double width;

	BarCal() : CalType(0), t0(0), beta(0), cbar(0), length(0), width(0) { }

	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);
};

class CalibEntry{
  public:
	TimeCal *timeCal;
	EnergyCal *energyCal;
	PositionCal *positionCal;
	BarCal *barCal;
	
	CalibEntry() : timeCal(NULL), energyCal(NULL), positionCal(NULL), barCal(NULL) { }
	
	CalibEntry(TimeCal *time_, EnergyCal *energy_, PositionCal *pos_, BarCal *bar_) : timeCal(time_), energyCal(energy_), positionCal(pos_), barCal(bar_) { }
	
	bool Time(){ return (timeCal != NULL); }
	
	bool Energy(){ return (energyCal != NULL); }
	
	bool Position(){ return (positionCal != NULL); }

	bool Bar(){ return (barCal != NULL); }
};

class CalibFile{
  private:
  	std::vector<TimeCal> time_calib;
  	std::vector<EnergyCal> energy_calib;
  	std::vector<PositionCal> position_calib;
	std::vector<BarCal> bar_calib;

  public:
	CalibFile(){ }
	
	CalibFile(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_);
	
	bool LoadTimeCal(const char *filename_);

	bool LoadEnergyCal(const char *filename_);
	
	bool LoadPositionCal(const char *filename_);

	bool LoadBarCal(const char *filename_);
	
	bool Load(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_);

	TimeCal *GetTimeCal(const unsigned int &id_);
	
	TimeCal *GetTimeCal(XiaData *event_);

	EnergyCal *GetEnergyCal(const unsigned int &id_);
	
	EnergyCal *GetEnergyCal(XiaData *event_);

	PositionCal *GetPositionCal(const unsigned int &id_);
	
	PositionCal *GetPositionCal(XiaData *event_);

	BarCal *GetBarCal(const unsigned int &id_);
	
	BarCal *GetBarCal(XiaData *event_);
	
	CalibEntry *GetCalibEntry(const unsigned int &id_);
	
	CalibEntry *GetCalibEntry(XiaData *event_);

	size_t GetMaxTime(){ return time_calib.size(); }

	size_t GetMaxEnergy(){ return energy_calib.size(); }

	size_t GetMaxPosition(){ return position_calib.size(); }

	size_t GetMaxBar(){ return bar_calib.size(); }

	void Debug(int mode);

	bool Write(TFile *f_);
};

extern CalibEntry dummyCalib;

unsigned int GetPixieID(const std::string &str_);

template<class T>
bool LoadCalibFile(const char *filename_, std::vector<T> &vec){
	std::ifstream calibfile(filename_);
	if(!calibfile.good()){
		errStr << "LoadCalibFile: ERROR! Failed to open calibration file '" << filename_ << "'!\n";
		return false;
	}

	int line_num = 1;
	int prevID = -1;
	int readID;
	std::vector<std::string> values;
	std::string line;
	while(true){
		getline(calibfile, line);
		
		if(calibfile.eof() || !calibfile.good())
			break;
		else if(line.empty() || line[0] == '#')
			continue;
		
		split_str(line, values, '\t');
		
		if(values.empty())
			continue;

		T tempCalEntry;
		readID = tempCalEntry.ReadPars(values);
		
		if(readID < 0 || readID <= prevID){
			warnStr << "LoadCalibFile: WARNING! On line " << line_num++ << " of calibration file, invalid id number (id=" << readID << ", prev=" << prevID << ").\n";
			continue;
		}
		else if(readID > prevID+1){
			for(int i = prevID+1; i < readID; i++){
				T tempCalEntry;
				tempCalEntry.id = i;
				vec.push_back(tempCalEntry);
			}
		}
		
		prevID = readID;
		line_num++;
		vec.push_back(tempCalEntry);
	}
	
	return true;
}

#endif
