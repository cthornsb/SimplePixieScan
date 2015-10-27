#ifndef MAPFILE_HPP
#define MAPFILE_HPP

#include <vector>
#include <stdlib.h>

#include "ParentClass.hpp"

class ChannelEvent;

struct MapEntry{
	unsigned int location;
	std::string type;
	std::string subtype;
	std::string tag;
	
	MapEntry(){ clear(); }

	MapEntry(const std::string &input_, const char delimiter_=':'){ set(input_, delimiter_); }
	
	MapEntry(const std::string &type_, const std::string &subtype_, const std::string &tag_){ set(type_, subtype_, tag_); }
	
	MapEntry(const MapEntry &other);

	bool operator == (const MapEntry &other);

	void get(std::string &type_, std::string &subtype_, std::string &tag_);
	
	void set(const std::string &input_, const char delimiter_=':');
	
	void set(const std::string &type_, const std::string &subtype_, const std::string &tag_);
	
	void clear();
	
	unsigned int increment();
	
	bool compare(const MapEntry &other);
	
	bool compare(const std::string &type_, const std::string &subtype_, const std::string &tag_);
	
	std::string print();
};

class MapFile : public ParentClass{
  private:
	static const int max_modules = 14;
	static const int max_channels = 16;
  
	MapEntry detectors[max_modules][max_channels];
	std::vector<MapEntry> types;
	int max_defined_module;
	
	void clear_entries();

	void parse_string(const std::string &input_, std::string &left, std::string &right, char &even_odd);
	
  public:
	MapFile();
  
	MapFile(const char *filename_);

	~MapFile(){ clear_entries(); }

	int GetMaxModule(){ return max_defined_module; }

	MapEntry *GetMapEntry(int mod_, int chan_);
	
	MapEntry *GetMapEntry(ChannelEvent *event_);
	
	std::vector<MapEntry> *GetTypes(){ return &types; }
	
	std::string GetType(int mod_, int chan_);
	
	std::string GetSubtype(int mod_, int chan_);
	
	std::string GetTag(int mod_, int chan_);
	
	bool Load(const char *filename_);

	void PrintAllEntries();
	
	void PrintAllTypes();
};

#endif
