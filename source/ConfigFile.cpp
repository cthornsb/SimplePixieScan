#include <fstream>

#include "ConfigFile.hpp"

void ConfigFile::parse_string(const std::string &input_, std::string &name, std::string &value){
	bool left_side = true;
	name = ""; value = "";
	for(size_t index = 0; index < input_.size(); index++){
		if(left_side){
			if(input_[index] == ':'){ 
				left_side = false; 
				continue;
			}
			name += input_[index];
		}
		else{ value += input_[index]; }
	}
}

ConfigFile::ConfigFile() : ParentClass("ConfigFile"){ 
	event_width = 0.5; // Default value of 500 ns		
}

ConfigFile::ConfigFile(const char *filename_) : ParentClass("ConfigFile"){ 
	event_width = 0.5; // Default value of 500 ns
	Load(filename_); 
}

bool ConfigFile::Load(const char *filename_){
	std::ifstream configfile(filename_);
	if(!configfile.good()){
		PrintError("Failed to open input configuration file!");
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
		
		if(values[0] == "EVENT_WIDTH"){ event_width = atof(values[1].c_str()); }
	}
	
	return true;
}
