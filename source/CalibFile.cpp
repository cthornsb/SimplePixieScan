#include <iostream>
#include <fstream>

#include "CalibFile.hpp"

#include "XiaData.hpp"

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
	double readRadius;
	double readTheta;
	double readPhi;
	double readt0;
	double readSlope;
	double readOffset;
	while(true){
		calibfile >> readID >> readRadius >> readTheta >> readPhi >> readt0 >> readSlope >> readOffset;
		if(calibfile.eof() || !calibfile.good()){ break; }
		if(readID < 0 || readID <= prevID){
			std::cout << "CalibFile: \033[1;33mWARNING! On line " << line_num++ << ", invalid id number (" << readID << "). Ignoring.\033[0m\n";
			continue;
		}
		else if(readID > prevID+1){
			for(int i = prevID; i < readID-1; i++)
				calib.push_back(CalibEntry(i, 0.5, 0.0, 0.0, 0.0, 1.0, 0.0));
		}
		prevID = readID;
		line_num++;
		calib.push_back(CalibEntry(readID, readRadius, readTheta, readPhi, readt0, readSlope, readOffset));
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
