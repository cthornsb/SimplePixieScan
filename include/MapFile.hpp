#ifndef MAPFILE_HPP
#define MAPFILE_HPP

#include <vector>
#include <stdlib.h>

#include "ParentClass.hpp"

struct MapEntry{
	std::string type;
	std::string subtype;
	std::string tag;
	
	MapEntry(){
		clear();
	}

	MapEntry(const std::string &input_, const char delimiter_=':'){
		set(input_, delimiter_);
	}
	
	MapEntry(const std::string &type_, const std::string &subtype_, const std::string &tag_){
		set(type_, subtype_, tag_);
	}
	
	void get(std::string &type_, std::string &subtype_, std::string &tag_){
		type_ = type; subtype_ = subtype; tag_ = tag;
	}
	
	void set(const std::string &input_, const char delimiter_=':');
	
	void set(const std::string &type_, const std::string &subtype_, const std::string &tag_){
		type = type_; subtype = subtype_; tag = tag_;
	}
	
	void clear(){
		type = "ignore"; subtype = ""; tag = "";
	}
	
	std::string print(){
		return (type+":"+subtype+":"+tag);
	}
};

class MapFile : public ParentClass{
  private:
	static const int max_modules = 14;
	static const int max_channels = 16;
  
	MapEntry detectors[max_modules][max_channels];
	std::vector<std::string> types;
	int max_defined_module;
	
	void clear_entries();

	void parse_string(const std::string &input_, std::string &left, std::string &right, char &even_odd);
	
  public:
	MapFile();
  
	MapFile(const char *filename_);

	~MapFile(){ clear_entries(); }

	int GetMaxModule(){ return max_defined_module; }

	MapEntry *GetMapEntry(int mod_, int chan_);
	
	std::vector<std::string> *GetTypes(){ return &types; }
	
	std::string GetType(int mod_, int chan_);
	
	std::string GetSubtype(int mod_, int chan_);
	
	std::string GetTag(int mod_, int chan_);
	
	bool Load(const char *filename_);

	void PrintAllEntries();
	
	void PrintAllTypes();
};

#endif
