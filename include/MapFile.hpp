#ifndef MAPFILE_HPP
#define MAPFILE_HPP

#include <vector>
#include <stdlib.h>

double absdiff(const double &v1, const double &v2);

class ChannelEvent;

class MapEntry{
  public:
	unsigned int location;
	std::string type;
	std::string subtype;
	std::string tag;
	std::vector<float> args;
	
	MapEntry(){ clear(); }

	MapEntry(const std::string &input_, const char delimiter_=':'){ set(input_, delimiter_); }
	
	MapEntry(const std::string &type_, const std::string &subtype_, const std::string &tag_){ set(type_, subtype_, tag_); }
	
	MapEntry(const MapEntry &other);

	bool operator == (const MapEntry &other);

	void get(std::string &type_, std::string &subtype_, std::string &tag_);
	
	void set(const std::string &input_, const char delimiter_=':');
	
	void set(const std::string &type_, const std::string &subtype_, const std::string &tag_);
	
	void pushArg(const float &arg_){ args.push_back(arg_); }
	
	void clear();
	
	unsigned int increment();
	
	bool getArg(const size_t &index_, float &arg);
	
	std::string print();
};

class MapFile{
  private:
  	bool init;

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
	
	bool IsInit(){ return init; }
	
	int GetFirstOccurance(const std::string &type_);
	
	int GetLastOccurance(const std::string &type_);
	
	bool Load(const char *filename_);

	void PrintAllEntries();
	
	void PrintAllTypes();
};

#endif
