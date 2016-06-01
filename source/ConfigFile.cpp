#include <fstream>
#include <sstream>
#include <iostream>

#include "TFile.h"
#include "TNamed.h"

#include "ConfigFile.hpp"

ConfigFile::ConfigFile(){ 
	eventWidth = 0.5; // Default value of 500 ns
	init = false;	
}

ConfigFile::ConfigFile(const char *filename_){ 
	eventWidth = 0.5; // Default value of 500 ns
	Load(filename_); 
}

bool ConfigFile::Load(const char *filename_){
	std::ifstream configfile(filename_);
	if(!configfile.good()){
		std::cout << "ConfigFile: \033[1;31mERROR! Failed to open input configuration file!\033[0m\n";
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
		
		if(values[0] == "eventWidth"){ eventWidth = atof(values[1].c_str()); }
	}
	
	return true;
}

bool ConfigFile::Write(TFile *f_){
	if(!f_ || !f_->IsOpen())
		return false;

	f_->mkdir("config");
	f_->cd("config");

	std::stringstream stream;
	stream << eventWidth << " us";
	TNamed named("eventWidth", stream.str().c_str());
	named.Write();
	
	return true;
}
