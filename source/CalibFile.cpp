#include <iostream>
#include <fstream>
#include <cmath>

#include "CalibFile.hpp"

#include "XiaData.hpp"
#include "ScanInterface.hpp"

CalibEntry dummyCalib(new TimeCal(), new EnergyCal());

TimeCal::TimeCal(const std::vector<std::string> &pars_) : id(0), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { 
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else if(index == 1) r0 = strtod(iter->c_str(), NULL);
		else if(index == 2) theta = strtod(iter->c_str(), NULL);
		else if(index == 3) phi = strtod(iter->c_str(), NULL);
		else if(index == 4) t0 = strtod(iter->c_str(), NULL);
		index++;
	}
}

void TimeCal::Print(){
	std::cout << " id=" << id << ", r0=" << r0 << ", theta=" << theta << ", phi=" << phi << ", t0=" << t0 << std::endl;
}

EnergyCal::EnergyCal(const std::vector<std::string> &pars_) : id(0) { 
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else vals.push_back(strtod(iter->c_str(), NULL));
		index++;
	}
}

bool EnergyCal::GetCalEnergy(const double &adc_, double &E){
	if(vals.empty()) return false;
	E = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		E += vals.at(i)*std::pow(adc_, i);
	return true;
}

double EnergyCal::GetCalEnergy(const double &adc_){
	if(vals.empty()) return adc_;
	double output = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		output += vals.at(i)*std::pow(adc_, i);
	return output;
}

void EnergyCal::Print(){
	int index = 0;
	std::cout << " id=" << id;
	for(std::vector<double>::iterator iter = vals.begin(); iter != vals.end(); iter++){
		std::cout << ", p" << index++ << "=" << (*iter);
	}
	std::cout << std::endl;
}

bool CalibFile::_load(const char *filename_, const bool &time_){
	std::ifstream calibfile(filename_);
	if(!calibfile.good()){
		if(time_)
			std::cout << "calibfile: \033[1;31mERROR! Failed to open input time calibration file!\033[0m\n";
		else
			std::cout << "calibfile: \033[1;31mERROR! Failed to open input energy calibration file!\033[0m\n";
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
			if(time_)
				std::cout << "CalibFile: \033[1;33mWARNING! On line " << line_num++ << " of time calibration file, invalid id number (" << readID << "). Ignoring.\033[0m\n";
			else
				std::cout << "CalibFile: \033[1;33mWARNING! On line " << line_num++ << " of energy calibration file, invalid id number (" << readID << "). Ignoring.\033[0m\n";
			continue;
		}
		else if(readID > prevID+1){
			for(int i = prevID+1; i < readID; i++)
				if(time_)
					time_calib.push_back(TimeCal(i, 0.5, 0.0, 0.0, 0.0));
				else
					energy_calib.push_back(EnergyCal(i));
		}
		
		prevID = readID;
		line_num++;
		if(time_)
			time_calib.push_back(TimeCal(values));
		else
			energy_calib.push_back(EnergyCal(values));			
	}

	return true;
}

CalibFile::CalibFile(const char *timeFilename_, const char *energyFilename_){ 
	Load(timeFilename_, energyFilename_);
}

bool CalibFile::LoadTimeCal(const char *filename_){
	return _load(filename_, true);
}

bool CalibFile::LoadEnergyCal(const char *filename_){
	return _load(filename_, false);
}

bool CalibFile::Load(const char *timeFilename_, const char *energyFilename_){
	return (LoadTimeCal(timeFilename_) && LoadEnergyCal(energyFilename_));
}

TimeCal *CalibFile::GetTimeCal(const unsigned int &id_){
	if(id_ >= time_calib.size()){ return NULL; }
	return &time_calib.at(id_);
}

TimeCal *CalibFile::GetTimeCal(XiaData *event_){
	return GetTimeCal(16*event_->modNum+event_->chanNum); 
}

EnergyCal *CalibFile::GetEnergyCal(const unsigned int &id_){
	if(id_ >= energy_calib.size()){ return NULL; }
	return &energy_calib.at(id_);
}

EnergyCal *CalibFile::GetEnergyCal(XiaData *event_){
	return GetEnergyCal(16*event_->modNum+event_->chanNum); 
}

CalibEntry *CalibFile::GetCalibEntry(const unsigned int &id_){
	TimeCal *tcal = this->GetTimeCal(id_);
	EnergyCal *ecal = this->GetEnergyCal(id_);
	return (new CalibEntry(tcal, ecal));
}

CalibEntry *CalibFile::GetCalibEntry(XiaData *event_){
	return GetCalibEntry(16*event_->modNum+event_->chanNum); 
}

void CalibFile::Debug(int mode){
	if(mode==0 || mode==2){
		std::cout << "Timing calibration:\n";
		for(size_t i = 0; i < time_calib.size(); i++)
			time_calib[i].Print();
	}
	if(mode==1 || mode==2){
		std::cout << "Energy calibration:\n";
		for(size_t i = 0; i < energy_calib.size(); i++)
			energy_calib[i].Print();
	}
}
