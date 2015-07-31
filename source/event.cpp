#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>

template <typename T>
std::string to_str(const T &input_){
	std::stringstream stream;
	stream << input_;
	return stream.str();
}

struct ChanEvent{
	double energy; /// Raw pixie energy
	double time; /// Raw low-res pixie time
	std::vector<unsigned int> trace; /// Trace capture
	
	static const int numQdcs = 8;	 /// Number of QDCs onboard
	unsigned int qdcValue[numQdcs];  /// QDCs from onboard

	int modNum; /// Module number
	int chanNum; /// Channel number
	unsigned int trigTime; /// The channel trigger time, trigger time and the lower 32 bits of the event time are not necessarily the same but could be separated by a constant value.
	unsigned int cfdTime; /// CFD trigger time in units of 1/256 pixie clock ticks
	unsigned int eventTimeLo; /// Lower 32 bits of pixie16 event time
	unsigned int eventTimeHi; /// Upper 32 bits of pixie16 event time

	bool virtualChannel; /// Flagged if generated virtually in Pixie DSP
	bool pileupBit; /// Pile-up flag from Pixie
	bool saturatedBit; /// Saturation flag from Pixie
};

struct MapEntry{
	int mod;
	int chan;
	std::string type;
	std::string subtype;
	
	MapEntry(const int &mod_, const int &chan_, const std::string &type_, const std::string &subtype_){
		mod = mod_; chan = chan_; type = type_; subtype = subtype_;
	}
	
	void Print(){ std::cout << mod << " " << chan << " " << type << ":" << subtype << std::endl; }
};

class ParentClass{
  protected:
	std::string class_name;
	bool debug_mode;
	bool init;
	
  public:
	ParentClass(std::string name_){ 
		class_name = name_; 
		debug_mode = false;
		init = false;
	}

	bool IsInit(){ return init; }
	
	void SetDebugMode(bool debug_=true){ debug_mode = debug_; }
	
	void PrintMsg(const std::string &msg_){ std::cout << class_name << ": " << msg_ << std::endl; }
	
	void PrintError(const std::string &msg_){ std::cout << "\e[1;31m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
	
	void PrintWarning(const std::string &msg_){ std::cout << "\e[1;33m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
	
	void PrintNote(const std::string &msg_){ std::cout << "\e[1;34m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
};

class MapFile : public ParentClass{
  private:
	std::vector<MapEntry*> entries;
	int max_module;
	
	void clear_entries(){
		for(std::vector<MapEntry*>::iterator iter = entries.begin(); iter != entries.end(); iter++){
			delete (*iter);
		}
		entries.clear();
	}

	void parse_string(const std::string &input_, std::string &left, std::string &right, char &even_odd){
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
	
  public:
	MapFile() : ParentClass("MapFile"){ 
		max_module = -9999;
	}
  
	MapFile(const char *filename_) : ParentClass("MapFile"){ 
		max_module = -9999;
		Load(filename_); 
	}

	~MapFile(){ clear_entries(); }

	int GetMaxModule(){ return max_module; }
	
	bool Load(const char *filename_){
		clear_entries();
	
		std::ifstream mapfile(filename_);
		if(!mapfile.good()){
			PrintError("Failed to open input map file!");
			return (init = false);
		}
		
		init = true;
		
		std::string line;
		std::string values[4];
		unsigned int line_num = 0;
		size_t current_value;
		bool reading;
		while(true){
			std::getline(mapfile, line);
			if(mapfile.eof() || !mapfile.good()){ break; }
			line_num++;

			values[0] = ""; values[1] = "";
			values[2] = ""; values[3] = "";
			current_value = 0;
			reading = false;
		
			for(size_t index = 0; index < line.size(); index++){
				if(line[index] == ' ' || line[index] == '\t' || line[index] == '\n'){ 
					if(reading){ 
						if(++current_value >= 4){ break; } 
						reading = false;
					}
					continue; 
				}
				else if(line[index] == '#'){ break; }
				else if(!reading){ reading = true; }
			
				values[current_value] += line[index];
			}

			int mod, chan;
			mod = atoi(values[0].c_str());
			std::string type = values[2];
			std::string subtype = values[3];
			
			if(mod > max_module){ max_module = mod; }

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
					MapEntry *entry = new MapEntry(mod, *iter, type, subtype);
					entries.push_back(entry);
				}
			}
			else{
				chan = atoi(values[1].c_str());
				MapEntry *entry = new MapEntry(mod, chan, type, subtype);
				entries.push_back(entry);
			}
		}
		
		mapfile.close();
		
		return init;
	}
	
	bool GetMapEntry(int mod_, int chan_, std::string &type, std::string &subtype){
		for(std::vector<MapEntry*>::iterator iter = entries.begin(); iter != entries.end(); iter++){
			if((*iter)->mod == mod_ && (*iter)->chan == chan_){ 
				type = (*iter)->type;
				subtype = (*iter)->subtype;
				return true; 
			}
		}
		return false;
	}
	
	void PrintAllEntries(){
		for(std::vector<MapEntry*>::iterator iter = entries.begin(); iter != entries.end(); iter++){
			(*iter)->Print();
		}
	}
};

class ConfigFile : public ParentClass{
  private:
	void parse_string(const std::string &input_, std::string &name, std::string &value){
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

  public:
	float event_width;
  
	ConfigFile() : ParentClass("ConfigFile"){ 
		event_width = 0.5; // Default value of 500 ns		
	}
	
	ConfigFile(const char *filename_) : ParentClass("ConfigFile"){ 
		event_width = 0.5; // Default value of 500 ns
		Load(filename_); 
	}
	
	bool Load(const char *filename_){
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
	}
};

int main(){
	MapFile mapfile("dummy.map");
	ConfigFile configfile("dummy.config");
	
	return 0;
}
