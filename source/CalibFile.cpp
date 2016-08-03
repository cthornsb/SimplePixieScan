#include <iostream>
#include <fstream>
#include <cmath>

#include "CalibFile.hpp"

#include "XiaData.hpp"
#include "ScanInterface.hpp"

CalibEntry::CalibEntry(const std::vector<std::string> &pars_) : id(0), r0(0.0), theta(0.0), phi(0.0), t0(0.0) { 
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else if(index == 1) r0 = strtod(iter->c_str(), NULL);
		else if(index == 2) theta = strtod(iter->c_str(), NULL);
		else if(index == 3) phi = strtod(iter->c_str(), NULL);
		else if(index == 4) t0 = strtod(iter->c_str(), NULL);
		else vals.push_back(strtod(iter->c_str(), NULL));
		index++;
	}
}

bool CalibEntry::GetCalEnergy(const double &adc_, double &E){
	if(vals.empty()) return false;
	E = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		E += vals.at(i)*std::pow(adc_, i);
	return true;
}

double CalibEntry::GetCalEnergy(const double &adc_){
	if(vals.empty()) return adc_;
	double output = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		output += vals.at(i)*std::pow(adc_, i);
	return output;
}

CalibFile::CalibFile(const char *filename_){ 
	Load(filename_); 
}
	
bool CalibFile::Load(const char *filename_){
	std::ifstream calibfile(filename_);
	if(!calibfile.good()){
		std::cout << "calibfile: \033[1;31mERROR! Failed to open input CalibEntry file!\033[0m\n";
		return (init = false);
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
			std::cout << "CalibFile: \033[1;33mWARNING! On line " << line_num++ << ", invalid id number (" << readID << "). Ignoring.\033[0m\n";
			continue;
		}
		else if(readID > prevID+1){
			for(int i = prevID; i < readID-1; i++)
				calib.push_back(CalibEntry(i, 0.5, 0.0, 0.0, 0.0));
		}
		
		prevID = readID;
		line_num++;
		calib.push_back(CalibEntry(values));
	}

	return (init=true);
}

CalibEntry *CalibFile::GetCalibEntry(const unsigned int &id_){
	if(id_ >= calib.size()){ return NULL; }
	return &calib.at(id_);
}

CalibEntry *CalibFile::GetCalibEntry(XiaData *event_){
	return GetCalibEntry(16*event_->modNum+event_->chanNum); 
}
