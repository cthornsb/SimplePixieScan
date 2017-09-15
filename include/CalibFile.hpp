#ifndef CALIBFILE_HPP
#define CALIBFILE_HPP

#include <vector>
#include <string>

#include "CTerminal.h"

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

	virtual void ReadPars(const std::vector<std::string> &pars_) { }
};

class PositionCal : public CalType {
  public:
	double r0;
	double theta;
	double phi;
	
	PositionCal() : CalType(0), r0(0.0), theta(0.0), phi(0.0) { }
	
	PositionCal(const int &id_) : CalType(id_), r0(0.0), theta(0.0), phi(0.0) { }
	
	PositionCal(const int &id_, const double &r0_, const double &theta_, const double &phi_) : 
		CalType(id_), r0(r0_), theta(theta_), phi(phi_) { }
		
	PositionCal(const std::vector<std::string> &pars_);
	
	virtual std::string Print(bool fancy=true);

	virtual void ReadPars(const std::vector<std::string> &pars_);
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

	virtual void ReadPars(const std::vector<std::string> &pars_);
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

	virtual void ReadPars(const std::vector<std::string> &pars_);

  private:
	std::vector<double> vals;
};

class CalibEntry{
  public:
	TimeCal *timeCal;
	EnergyCal *energyCal;
	PositionCal *positionCal;
	
	CalibEntry() : timeCal(NULL), energyCal(NULL), positionCal(NULL) { }
	
	CalibEntry(TimeCal *time_, EnergyCal *energy_, PositionCal *pos_) : timeCal(time_), energyCal(energy_), positionCal(pos_) { }
	
	bool Time(){ return (timeCal != NULL); }
	
	bool Energy(){ return (energyCal != NULL); }
	
	bool Position(){ return (positionCal != NULL); }
};

class CalibFile{
  private:
  	std::vector<TimeCal> time_calib;
  	std::vector<EnergyCal> energy_calib;
  	std::vector<PositionCal> position_calib;

  public:
	CalibFile(){ }
	
	CalibFile(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_);
	
	bool LoadTimeCal(const char *filename_);

	bool LoadEnergyCal(const char *filename_);
	
	bool LoadPositionCal(const char *filename_);
	
	bool Load(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_);

	TimeCal *GetTimeCal(const unsigned int &id_);
	
	TimeCal *GetTimeCal(XiaData *event_);

	EnergyCal *GetEnergyCal(const unsigned int &id_);
	
	EnergyCal *GetEnergyCal(XiaData *event_);

	PositionCal *GetPositionCal(const unsigned int &id_);
	
	PositionCal *GetPositionCal(XiaData *event_);
	
	CalibEntry *GetCalibEntry(const unsigned int &id_);
	
	CalibEntry *GetCalibEntry(XiaData *event_);
	
	void Debug(int mode);

	bool Write(TFile *f_);
};

extern CalibEntry dummyCalib;

template<class T>
bool LoadCalibFile(const char *filename_, std::vector<T> &vec){
	std::ifstream calibfile(filename_);
	if(!calibfile.good()){
		std::cout << "LoadCalibFile: \033[1;31mERROR! Failed to open calibration file '" << filename_ << "'!\033[0m\n";
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
		
		readID = (int)strtol(values[0].c_str(), NULL, 0);
		
		if(readID < 0 || readID <= prevID){
			std::cout << "LoadCalibFile: \033[1;33mWARNING! On line " << line_num++ << " of calibration file, invalid id number (" << readID << "). Ignoring.\033[0m\n";
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
		T tempCalEntry;
		tempCalEntry.ReadPars(values);
		vec.push_back(tempCalEntry);
	}
	
	return true;
}

#endif
