#include <iostream>
#include <fstream>

#include "CalibFile.hpp"

CalibFile::CalibFile(const char *filename_){ 
	Load(filename_); 
}
	
bool CalibFile::Load(const char *filename_){
	std::ifstream calibfile(filename_);
	if(!calibfile.good()){
		std::cout << "calibfile: \033[1;31mERROR! Failed to open input calibration file!\033[0m\n";
		return (init = false);
	}

	int readID;
	double readInter;
	double readSlope;
	while(true){
		calibfile >> readID >> readInter >> readSlope;
		if(calibfile.eof() || !calibfile.good()){ break; }
		calib.push_back(calibration(readID, readInter, readSlope));
	}

	return (init=true);
}

double CalibFile::GetEnergy(const int &id_, const double &adc_){
	for(std::vector<calibration>::iterator iter = calib.begin(); iter != calib.end(); iter++){
		if(iter->id == id_)
			return iter->getEnergy(adc_);
	}
	return adc_;
}
