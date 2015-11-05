#include <fstream>
#include <sstream>

#include "ChannelEvent.hpp"

#include "MapFile.hpp"

std::string to_str(const int &input_){
	std::stringstream stream; stream << input_;
	return stream.str();
}

MapEntry::MapEntry(const MapEntry &other){
	type = other.type; 
	subtype = other.subtype; 
	tag = other.tag; 
	location = other.location;
	beta = other.beta;
	gamma = other.gamma;
}

bool MapEntry::operator == (const MapEntry &other){
	if(other.type == type && other.subtype == subtype && other.tag == tag){ return true; }
	return false;
}

void MapEntry::get(std::string &type_, std::string &subtype_, std::string &tag_){
	type_ = type; 
	subtype_ = subtype; 
	tag_ = tag;
}

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
	
void MapEntry::set(const std::string &type_, const std::string &subtype_, const std::string &tag_){
	type = type_; 
	subtype = subtype_; 
	tag = tag_;
}

void MapEntry::clear(){
	location = 0; 
	type = "ignore"; 
	subtype = ""; 
	tag = "";
	beta = -1.0;
	gamma = -1.0;
}

unsigned int MapEntry::increment(){
	return ++location;
}

std::string MapEntry::print(){
	return (type+":"+subtype+":"+tag);
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

MapFile::MapFile(){ 
	max_defined_module = -9999;
	init = false;
}

MapFile::MapFile(const char *filename_){ 
	max_defined_module = -9999;
	Load(filename_); 
}

MapEntry *MapFile::GetMapEntry(int mod_, int chan_){
	if(mod_ >= max_modules || chan_ >= max_channels){ return NULL; }
	return &detectors[mod_][chan_];
}

MapEntry *MapFile::GetMapEntry(ChannelEvent *event_){
	if(event_->modNum >= max_modules || event_->chanNum >= max_channels){ return NULL; }
	return &detectors[event_->modNum][event_->chanNum];
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
		std::cout << "MapFile: \033[1;31mERROR! Failed to open input map file!\033[0m\n";
		return (init = false);
	}
	
	init = true;
	
	std::string line;
	std::string values[5];
	float beta, gamma;
	int line_num = 0;
	size_t current_value;
	bool reading;
	while(true){
		std::getline(mapfile, line);
		if(mapfile.eof() || !mapfile.good()){ break; }
		if(line[0] == '#'){ continue; }
		line_num++;

		values[0] = ""; // module
		values[1] = ""; // channel
		values[2] = ""; // type:subtype:tag
		values[3] = ""; // beta
		values[4] = ""; // gamma
		beta = -1.0;
		gamma = -1.0;
		current_value = 0;
		reading = false;
	
		for(size_t index = 0; index < line.size(); index++){
			if(line[index] == ' ' || line[index] == '\t' || line[index] == '\n'){ 
				if(reading){ 
					if(++current_value >= 5){ break; } 
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
			std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", invalid module number. Ignoring.\033[0m\n";
			continue;
		}
		
		if(mod > max_defined_module){ max_defined_module = mod; }

		if(values[0].find(':') != std::string::npos){
			std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", the ':' wildcard is not permitted for specification of modules.\033[0m\n";
			init = false;
			break;
		}

		if(values[3] != ""){ // Manually set beta
			beta = atof(values[3].c_str());
		}
		
		if(values[4] != ""){ // Manually set gamma
			gamma = atof(values[4].c_str());
		}

		if(values[1].find(':') != std::string::npos){
			std::vector<int> channels;
			std::string lhs, rhs;
			char leftover;
			
			parse_string(values[1], lhs, rhs, leftover);
			int start_chan = atoi(lhs.c_str());
			int stop_chan = atoi(rhs.c_str());
			
			if(start_chan > stop_chan){ // Flip the start and stop channels
				std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", start channel > stop channel. I'm assuming you swapped the values.\033[0m\n";
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
				if(leftover != 0x0){ std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", only even (e) or odd (o) may be specified as channel wildcards.\033[0m\n"; }
				for(int i = start_chan; i <= stop_chan; i++){
					channels.push_back(i);
				}
			}
			
			for(std::vector<int>::iterator iter = channels.begin(); iter != channels.end(); iter++){
				if(*iter >= max_channels){
					std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", invalid channel number. Ignoring.\033[0m\n";
					break;
				}
				detectors[mod][*iter].set(values[2]);
				detectors[mod][*iter].setBeta(beta);
				detectors[mod][*iter].setGamma(gamma);
				
				bool in_list = false;
				for(std::vector<MapEntry>::iterator iter2 = types.begin(); iter2 != types.end(); iter2++){
					if(*iter2 == detectors[mod][*iter]){
						detectors[mod][*iter].location = iter2->increment();
						in_list = true;
						break;
					}
				}
				if(!in_list){ 
					types.push_back(MapEntry(detectors[mod][*iter])); 
				}
			}
		}
		else{
			chan = (unsigned)atoi(values[1].c_str());
			if(chan >= max_modules){
				std::cout << "MapFile: \033[1;33mWARNING! On line " << line_num << ", invalid channel number. Ignoring.\033[0m\n";
				continue;
			}
			detectors[mod][chan].set(values[2]);
			detectors[mod][chan].setBeta(beta);
			detectors[mod][chan].setGamma(gamma);
			
			bool in_list = false;
			for(std::vector<MapEntry>::iterator iter = types.begin(); iter != types.end(); iter++){
				if(*iter == detectors[mod][chan]){
					detectors[mod][chan].location = iter->increment();
					in_list = true;
					break;
				}
			}
			if(!in_list){ 
				types.push_back(MapEntry(detectors[mod][chan])); 
			}
		}
	}
	
	mapfile.close();
	
	return init;
}

void MapFile::PrintAllEntries(){
	std::cout << "MapFile: List of defined detectors...\n";
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(detectors[i][j].type == "ignore"){ continue; }
			std::cout << " " << i << ", " << j << ", " << detectors[i][j].location << " " << detectors[i][j].print() << std::endl;
		}
	}
}

void MapFile::PrintAllTypes(){
	for(std::vector<MapEntry>::iterator iter = types.begin(); iter != types.end(); iter++){
		if(iter->type == "ignore"){ continue; }
		std::cout << " " << iter->print() << std::endl;
	}
}
