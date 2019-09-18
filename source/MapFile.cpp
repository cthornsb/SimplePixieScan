#include <fstream>
#include <sstream>

#include "TFile.h"
#include "TObjString.h"

#include "XiaData.hpp"

#include "MapFile.hpp"
#include "ColorTerm.hpp"

double absdiff(const double &v1, const double &v2){
	return (v1 >= v2)?(v1-v2):(v2-v1);
}

std::string to_str(const int &input_){
	std::stringstream stream; stream << input_;
	return stream.str();
}

///////////////////////////////////////////////////////////////////////////////
// class MapEntry
///////////////////////////////////////////////////////////////////////////////

MapEntry::MapEntry(const MapEntry &other){
	type = other.type; 
	subtype = other.subtype; 
	tag = other.tag; 
	location = other.location;
	args = other.args;
}

bool MapEntry::operator == (const MapEntry &other) const {
	if(other.type == type && other.subtype == subtype && other.tag == tag){ return true; }
	return false;
}

void MapEntry::get(std::string &type_, std::string &subtype_, std::string &tag_) const {
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
}

bool MapEntry::getArg(const size_t &index_, double &arg) const {
	if(index_ >= args.size()){ return false; }
	arg = args.at(index_);
	return true;
}

bool MapEntry::hasTag(const std::string &tag_) const {
	return (tag.find(tag_) != std::string::npos);
}

unsigned int MapEntry::increment(){
	return ++location;
}

std::string MapEntry::print() const {
	std::stringstream output; 
	output << type << ":" << subtype << ":" << tag;
	for(std::vector<double>::const_iterator iter = args.begin(); iter != args.end(); ++iter)
		output << " " << *iter;
	return output.str();
}

///////////////////////////////////////////////////////////////////////////////
// class MapEntryValidator
///////////////////////////////////////////////////////////////////////////////
	
void MapEntryValidator::SetValid(const std::string &types, const std::string &subtypes/*=""*/, const std::string &tags/*=""*/){
	validTypes = types;
	validSubtypes = subtypes;
	validTags = tags;
}

void MapEntryValidator::SetInvalid(const std::string &types, const std::string &subtypes/*=""*/, const std::string &tags/*=""*/){
	invalidTypes = types;
	invalidSubtypes = subtypes;
	invalidTags = tags;
}

bool MapEntryValidator::Validate(const MapEntry &entry) const {
	if(validation == NO_VALIDATION) // No validation
		return true;

	int retval = PASSED;

	// Check types
	if(!entry.type.empty()){
		if(CheckFlag(NO_TYPE)) // No types allowed
			retval |= BAD_TYPE;
		if(CheckFlag(WITH_TYPE) && (validTypes.find(entry.type) == std::string::npos)) // With a type
			retval |= BAD_TYPE;
		if(CheckFlag(WITHOUT_TYPE) && (invalidTypes.find(entry.type) != std::string::npos)) // Without a type
			retval |= BAD_TYPE;
	}
	else if(CheckFlag(FORCE_TYPE))
		retval |= BAD_TYPE;

	// Check subtypes
	if(!entry.subtype.empty()){
		if(CheckFlag(NO_SUBTYPE)) // No subtypes allowed
			retval |= BAD_SUBTYPE;
		if(CheckFlag(WITH_SUBTYPE) && (validSubtypes.find(entry.subtype) == std::string::npos)) // With a subtype
			retval |= BAD_SUBTYPE;
		if(CheckFlag(WITHOUT_SUBTYPE) && (invalidSubtypes.find(entry.subtype) != std::string::npos)) // Without a subtype
			retval |= BAD_SUBTYPE;
	}
	else if(CheckFlag(FORCE_SUBTYPE))
		retval |= BAD_SUBTYPE;

	// Check tags
	if(!entry.tag.empty()){
		if(CheckFlag(NO_TAG)) // No tags allowed
			retval |= BAD_TAG;
		if(CheckFlag(WITH_TAG) && (validTags.find(entry.tag) == std::string::npos)) // With a tag
			retval |= BAD_TAG;
		if(CheckFlag(WITHOUT_TAG) && (invalidTags.find(entry.tag) != std::string::npos)) // Without a tag
			retval |= BAD_TAG;
	}
	else if(CheckFlag(FORCE_TAG))
		retval |= BAD_TAG;
	
	return (retval == PASSED);
}

void MapEntryValidator::Print() const {
	std::cout << "-----------------------------\n";
	if(CheckFlag(NO_TYPE))
		std::cout << " types allowed - NO\n";
	else{
		std::cout << " types allowed - YES\n";
		if(CheckFlag(WITH_TYPE))
			std::cout << "  valid  : \"" << validTypes << "\"\n";
		if(CheckFlag(WITHOUT_TYPE))
			std::cout << "  invalid: \"" << invalidTypes << "\"\n";
	}
	if(CheckFlag(NO_SUBTYPE))
		std::cout << " subtypes allowed - NO\n";
	else{
		std::cout << " subtypes allowed - YES\n";
		if(CheckFlag(WITH_SUBTYPE))
			std::cout << "  valid  : \"" << validSubtypes << "\"\n";
		if(CheckFlag(WITHOUT_SUBTYPE))
		std::cout << "  invalid: \"" << invalidSubtypes << "\"\n";
	}
	if(CheckFlag(NO_TAG))
		std::cout << " tags allowed - NO\n";
	else{
		std::cout << " tags allowed - YES\n";
		if(CheckFlag(WITH_TAG))
			std::cout << "  valid  : \"" << validTags << "\"\n";
		if(CheckFlag(WITHOUT_TAG))
			std::cout << "  invalid: \"" << invalidTags << "\"\n";
	}
	std::cout << "-----------------------------\n\n";
}

///////////////////////////////////////////////////////////////////////////////
// class MapFile
///////////////////////////////////////////////////////////////////////////////

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

MapEntry *MapFile::GetMapEntry(XiaData *event_){
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

void MapFile::GetModChan(const int &location, int &mod, int &chan){
	mod = location / max_channels;
	chan = location % max_channels;
}

void MapFile::GetListOfLocations(std::vector<int> &list, const MapEntryValidator &valid){
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(detectors[i][j].type == "ignore")
				continue;
			if(valid.Validate(detectors[i][j]))
				list.push_back(detectors[i][j].location);
		}
	}
}

int MapFile::GetFirstOccurance(const std::string &type_){
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(detectors[i][j].type == type_){ return (int)detectors[i][j].location; }
		}
	}
	return -1;
}

int MapFile::GetLastOccurance(const std::string &type_){
	for(int i = max_modules-1; i >= 0; i--){
		for(int j = max_channels-1; j >= 0; j--){
			if(detectors[i][j].type == type_){ return (int)detectors[i][j].location; }
		}
	}
	return -1;
}

int MapFile::GetAllOccurances(const std::string &type_, std::vector<int> &locations, const bool &isSingleEnded/*=true*/){
	int retval = 0;
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(!isSingleEnded && j % 2 != 0) continue; // Handle bar-type detectors
			if(detectors[i][j].type == type_){
				locations.push_back(detectors[i][j].location);
				retval++;
			}
		}
	}
	return retval;
}

bool MapFile::GetFirstStart(int &mod, int &chan){
	for(mod = 0; mod < max_modules; mod++){
		for(chan = 0; chan < max_channels; chan++){
			if(detectors[mod][chan].hasTag("start")) return true;
		}
	}
	return false;
}

bool MapFile::Load(const char *filename_){
	clear_entries();

	std::ifstream mapfile(filename_);
	if(!mapfile.good()){
		errStr << "MapFile: ERROR! Failed to open input map file!\n";
		return (init = false);
	}
	
	init = true;

	// Set the location of all possible detectors.
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			detectors[i][j].location = i*max_channels + j;
		}
	}
	
	std::string line;
	std::string argument;
	std::vector<std::string> values;
	int line_num = 0;
	bool reading;
	while(true){
		std::getline(mapfile, line);
		if(mapfile.eof() || !mapfile.good()){ break; }

		line_num++;
		if(line.empty() || line[0] == '#') // Check for empty lines and comments.
			continue; 
		
		values.clear();
		argument = "";
		reading = false;
	
		// Split the string into individual arguments.
		for(size_t index = 0; index < line.size(); index++){
			if(line[index] == ' ' || line[index] == '\t' || line[index] == '\n'){ 
				if(reading){ 
					values.push_back(argument);
					argument = "";
					reading = false; 
				}
				continue; 
			}
			else if(line[index] == '#'){ break; }
			else if(!reading){ reading = true; }
		
			argument += line[index];
		}
		
		// Check for a remaining argument.
		if(!argument.empty()){ values.push_back(argument); }

		// Check for two few map file arguments.
		if(values.size() < 3){
			warnStr << "MapFile: WARNING! On line " << line_num << ", expected at least 3 parameters but received " << values.size() << ". Ignoring.\n";
			continue;
		}

		// Check for the ':' character in the module specification.
		if(values.at(0).find(':') != std::string::npos){
			errStr << "MapFile: ERROR! On line " << line_num << ", the ':' wildcard is not permitted for specification of modules.\n";
			init = false;
			break;
		}

		int mod, chan;
		mod = (unsigned)atoi(values.at(0).c_str());
		if(mod >= max_modules){
			warnStr << "MapFile: WARNING! On line " << line_num << ", invalid module number (" << mod << "). Ignoring.\n";
			continue;
		}
		
		// Check for the maximum defined module.
		if(mod > max_defined_module){ max_defined_module = mod; }

		// Check for the ':' character in the channel specification.
		if(values.at(1).find(':') != std::string::npos){ // User has specified a range of channels.
			std::vector<int> channels;
			std::string lhs, rhs;
			char leftover;
			
			parse_string(values.at(1), lhs, rhs, leftover);
			int start_chan = atoi(lhs.c_str());
			int stop_chan = atoi(rhs.c_str());
			
			// Flip the start and stop channels
			if(start_chan > stop_chan){
				warnStr << "MapFile: WARNING! On line " << line_num << ", start channel > stop channel. I'm assuming you swapped the values.\n";
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
				if(leftover != 0x0){ warnStr << "MapFile: WARNING! On line " << line_num << ", only even (e) or odd (o) may be specified as channel wildcards.\n"; }
				for(int i = start_chan; i <= stop_chan; i++){
					channels.push_back(i);
				}
			}
			
			// Iterate over all specified channels and set the detector type.
			for(std::vector<int>::iterator iter = channels.begin(); iter != channels.end(); iter++){
				if(*iter >= max_channels){
					warnStr << "MapFile: WARNING! On line " << line_num << ", invalid channel number (" << *iter << "). Ignoring.\n";
					break;
				}
				detectors[mod][*iter].set(values.at(2));
				for(size_t arg_index = 3; arg_index < values.size(); arg_index++){
					detectors[mod][*iter].pushArg(strtod(values.at(arg_index).c_str(), NULL));
				}
				
				bool in_list = false;
				for(std::vector<std::string>::iterator iter2 = types.begin(); iter2 != types.end(); iter2++){
					if(*iter2 == detectors[mod][*iter].type){
						in_list = true;
						break;
					}
				}
				if(!in_list){ 
					types.push_back(detectors[mod][*iter].type); 
				}
			}
		}
		else{ // User has specified a single channel.
			chan = (unsigned)atoi(values.at(1).c_str());
			if(chan >= max_channels){
				warnStr << "MapFile: WARNING! On line " << line_num << ", invalid channel number (" << chan << "). Ignoring.\n";
				continue;
			}
			detectors[mod][chan].set(values.at(2));
			for(size_t arg_index = 3; arg_index < values.size(); arg_index++){
				detectors[mod][chan].pushArg(strtod(values.at(arg_index).c_str(), NULL));
			}
			
			bool in_list = false;
			for(std::vector<std::string>::iterator iter = types.begin(); iter != types.end(); iter++){
				if(*iter == detectors[mod][chan].type){
					in_list = true;
					break;
				}
			}
			if(!in_list){ 
				types.push_back(detectors[mod][chan].type); 
			}
		}
	}
	
	mapfile.close();
	
	// Check for at least one start detector.
	bool validStart = false;
	for(int i = 0; i < max_modules; i++){
		for(int j = 0; j < max_channels; j++){
			if(detectors[i][j].hasTag("start")){
				validStart = true;
				break;
			}
		}
	}	
	
	if(!validStart)
		warnStr << "MapFile: WARNING! Found no start detector!\n";
	
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
	for(std::vector<std::string>::iterator iter = types.begin(); iter != types.end(); iter++){
		if(*iter == "ignore"){ continue; }
		std::cout << " " << *iter << std::endl;
	}
}

bool MapFile::Write(TFile *f_){
	if(!f_ || !f_->IsOpen())
		return false;
		
	// Add all map entries to the output root file.
	const int num_mod = GetMaxModules();
	const int num_chan = GetMaxChannels();
	MapEntry *entryptr;

	std::string *dir_names = new std::string[num_mod];
	std::string *chan_names = new std::string[num_chan];
	for(int i = 0; i < num_mod; i++){
		std::stringstream stream;
		if(i < 10){ stream << "0" << i; }
		else{ stream << i; }
		dir_names[i] = "map/mod" + stream.str();
	}
	for(int i = 0; i < num_chan; i++){
		std::stringstream stream;
		if(i < 10){ stream << "0" << i; }
		else{ stream << i; }
		chan_names[i] = "chan" + stream.str();
	}

	f_->mkdir("map");
	for(int i = 0; i < num_mod; i++){
		bool first_good_channel = true;
		for(int j = 0; j < num_chan; j++){
			entryptr = GetMapEntry(i, j);
			if(entryptr->type == "ignore"){ continue; }
			if(first_good_channel){
				f_->mkdir(dir_names[i].c_str());
				f_->cd(dir_names[i].c_str());
				first_good_channel = false;		
			}
			std::stringstream stream;
			stream << i << " " << j << " " << entryptr->print();
			TObjString str(stream.str().c_str());
			str.Write();
		}
	}
	
	delete[] dir_names;
	delete[] chan_names;
	
	return true;
}
