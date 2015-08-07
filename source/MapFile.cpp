#include <fstream>

#include "MapFile.hpp"
#include "Unpacker.hpp"

void MapEntry::set(const std::string &input_, const char delimiter_/*=':'*/){
	int count = 0;
	type = ""; subtype = ""; tag = "";
	for(size_t index = 0; index < input_.size(); index++){
		if(input_[index] == delimiter_){
			count++;
			continue;
		}
		else if(count == 0){ type += input_[index]; }
		else if(count == 1){ subtype += input_[index]; }
		else if(count == 2){ tag += input_[index]; }
		else{ break; }
	}
}

void MapFile::clear_entries(){
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			detectors[i][j].clear();
		}
	}
	types.clear();
}

void MapFile::parse_string(const std::string &input_, std::string &left, std::string &right, char &even_odd){
	bool left_side = true;
	left = ""; right = ""; even_odd = 0x0;
	for(size_t index = 0; index < input_.size(); index++){
		if(left_side){
			if(input_[index] == ':'){ 
				left_side = false; 
				continue;
			}
			left += input_[index];
		}
		else{ 
			if(input_[index] == ':'){ 
				if(index++ >= input_.size()){ break; }
				even_odd = input_[index];
				return;
			}
			right += input_[index]; 
		}
	}
}

MapFile::MapFile() : ParentClass("MapFile"){ 
	max_defined_module = -9999;
}

MapFile::MapFile(const char *filename_) : ParentClass("MapFile"){ 
	max_defined_module = -9999;
	Load(filename_); 
}

MapEntry *MapFile::GetMapEntry(int mod_, int chan_){
	if(mod_ >= max_modules || chan_ >= max_channels){ return NULL; }
	return &detectors[mod_][chan_];
}

std::string MapFile::GetType(int mod_, int chan_){
	if(mod_ >= max_modules || chan_ >= max_channels){ return ""; }
	return detectors[mod_][chan_].type;
}

std::string MapFile::GetSubtype(int mod_, int chan_){
	if(mod_ >= max_modules || chan_ >= max_channels){ return ""; }
	return detectors[mod_][chan_].subtype;
}

std::string MapFile::GetTag(int mod_, int chan_){
	if(mod_ >= max_modules || chan_ >= max_channels){ return ""; }
	return detectors[mod_][chan_].tag;
}

bool MapFile::Load(const char *filename_){
	clear_entries();

	std::ifstream mapfile(filename_);
	if(!mapfile.good()){
		PrintError("Failed to open input map file!");
		return (init = false);
	}
	
	init = true;
	
	std::string line;
	std::string values[3];
	int line_num = 0;
	size_t current_value;
	bool reading;
	while(true){
		std::getline(mapfile, line);
		if(mapfile.eof() || !mapfile.good()){ break; }
		if(line[0] == '#'){ continue; }
		line_num++;

		values[0] = "";
		values[1] = "";
		values[2] = "";
		current_value = 0;
		reading = false;
	
		for(size_t index = 0; index < line.size(); index++){
			if(line[index] == ' ' || line[index] == '\t' || line[index] == '\n'){ 
				if(reading){ 
					if(++current_value >= 3){ break; } 
					reading = false;
				}
				continue; 
			}
			else if(line[index] == '#'){ break; }
			else if(!reading){ reading = true; }
		
			values[current_value] += line[index];
		}

		int mod, chan;
		mod = (unsigned)atoi(values[0].c_str());
		if(mod >= max_modules){
			PrintWarning("On line " + to_str(line_num) + ", invalid module number. Ignoring.");
			continue;
		}
		
		if(mod > max_defined_module){ max_defined_module = mod; }

		if(values[0].find(':') != std::string::npos){
			PrintError("On line " + to_str(line_num) + ", the ':' wildcard is not permitted for specification of modules!");
			init = false;
			break;
		}

		if(values[1].find(':') != std::string::npos){
			std::vector<int> channels;
			std::string lhs, rhs;
			char leftover;
			
			parse_string(values[1], lhs, rhs, leftover);
			int start_chan = atoi(lhs.c_str());
			int stop_chan = atoi(rhs.c_str());
			
			if(start_chan > stop_chan){ // Flip the start and stop channels
				PrintWarning("On line " + to_str(line_num) + ", start channel > stop channel. I'm assuming you accidentally swapped the values.");
				int dummy = start_chan;
				start_chan = stop_chan;
				stop_chan = dummy;
			}
			
			if(leftover == 0x65){ // 'e' even channels only
				if(start_chan % 2 != 0){ start_chan++; }
				for(int i = start_chan; i <= stop_chan; i+=2){
					channels.push_back(i);
				}
			}
			else if(leftover == 0x6F){ // 'o' odd channels only
				if(start_chan % 2 == 0){ start_chan++; }
				for(int i = start_chan; i <= stop_chan; i+=2){
					channels.push_back(i);
				}
			}
			else{ // All channels
				if(leftover != 0x0){ PrintWarning("On line " + to_str(line_num) + ", only even (e) or odd (o) may be specified as channel wildcards."); }
				for(int i = start_chan; i <= stop_chan; i++){
					channels.push_back(i);
				}
			}
			
			for(std::vector<int>::iterator iter = channels.begin(); iter != channels.end(); iter++){
				if(*iter >= max_channels){
					PrintWarning("On line " + to_str(line_num) + ", invalid channel number. Ignoring.");
					break;
				}
				detectors[mod][*iter].set(values[2]);
				
				bool in_list = false;
				for(std::vector<DetType>::iterator iter2 = types.begin(); iter2 != types.end(); iter2++){
					if(iter2->type == detectors[mod][*iter].type){
						detectors[mod][*iter].location = iter2->count++;
						in_list = true;
						break;
					}
				}
				if(!in_list){ 
					types.push_back(DetType(detectors[mod][*iter].type)); 
					detectors[mod][*iter].location = 0;
				}
			}
		}
		else{
			chan = (unsigned)atoi(values[1].c_str());
			if(chan >= max_modules){
				PrintWarning("On line " + to_str(line_num) + ", invalid channel number. Ignoring.");
				continue;
			}
			detectors[mod][chan].set(values[2]);
			
			bool in_list = false;
			for(std::vector<DetType>::iterator iter = types.begin(); iter != types.end(); iter++){
				if(iter->type == detectors[mod][chan].type){
					detectors[mod][chan].location = iter->count++;
					in_list = true;
					break;
				}
			}
			if(!in_list){ 
				types.push_back(DetType(detectors[mod][chan].type)); 
				detectors[mod][chan].location = 0;
			}
		}
	}
	
	mapfile.close();
	
	return init;
}

void MapFile::PrintAllEntries(){
	PrintMsg("List of defined detectors...");
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(detectors[i][j].type == "ignore"){ continue; }
			std::cout << " " << i << ", " << j << ", " << detectors[i][j].location << " " << detectors[i][j].print() << std::endl;
		}
	}
}

void MapFile::PrintAllTypes(){
	for(std::vector<DetType>::iterator iter = types.begin(); iter != types.end(); iter++){
		if(iter->type == "ignore"){ continue; }
		std::cout << " " << iter->type << ": " << iter->count << std::endl;
	}
}
