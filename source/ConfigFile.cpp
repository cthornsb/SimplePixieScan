#include <fstream>
#include <sstream>
#include <iostream>

#include "TFile.h"
#include "TObjString.h"

#include "ConfigFile.hpp"
#include "ColorTerm.hpp"

ConfigFile::ConfigFile() : adcClock(4E-9), sysClock(8E-9), eventWidth(0.5), eventDelay(0.0), buildMethod(0), init(false) { 
}

ConfigFile::ConfigFile(const char *filename_) : adcClock(4E-9), sysClock(8E-9), eventWidth(0.5), eventDelay(0.0), buildMethod(0), init(false) { 
	Load(filename_); 
}

bool ConfigFile::Load(const char *filename_){
	std::ifstream configfile(filename_);
	if(!configfile.good()){
		errStr << "ConfigFile: ERROR! Failed to open input configuration file!\n";
		return (init = false);
	}
	
	init = true;
	
	std::string line;
	std::string values[2];
	unsigned int line_num = 0;
	size_t current_value;
	bool reading;
	while(true){
		std::getline(configfile, line);
		if(configfile.eof() || !configfile.good()){ break; }
		line_num++;

		values[0] = ""; values[1] = "";
		current_value = 0;
		reading = false;
	
		for(size_t index = 0; index < line.size(); index++){
			if(line[index] == ' ' || line[index] == '\t' || line[index] == '\n'){ 
				if(reading){ 
					if(++current_value >= 2){ break; } 
					reading = false;
				}
				continue; 
			}
			else if(line[index] == '#'){ break; }
			else if(!reading){ reading = true; }
		
			values[current_value] += line[index];
		}
		
		if(values[0] == "adcClock"){ adcClock = strtod(values[1].c_str(), NULL); }
		else if(values[0] == "sysClock"){ sysClock = strtod(values[1].c_str(), NULL); }
		else if(values[0] == "eventWidth"){ eventWidth = strtod(values[1].c_str(), NULL); }
		else if(values[0] == "eventDelay"){ eventDelay = strtod(values[1].c_str(), NULL); }
		else if(values[0] == "buildMethod"){ buildMethod = strtol(values[1].c_str(), NULL, 0); }
	}
	
	return true;
}

bool ConfigFile::Write(TFile *f_){
	if(!f_ || !f_->IsOpen())
		return false;

	f_->mkdir("config");
	f_->cd("config");

	std::stringstream stream[5];
	stream[0] << "adcClock " << adcClock << " s";
	stream[1] << "sysClock " << sysClock << " s";
	stream[2] << "eventWidth " << eventWidth << " us";
	stream[3] << "eventDelay " << eventDelay << " us";
	stream[4] << "buildMethod " << buildMethod;
	
	TObjString strings[5];
	for(size_t i = 0; i < 5; i++){
		TObjString str(stream[i].str().c_str());
		str.Write();
	}

	return true;
}
