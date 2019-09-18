#ifndef MAPFILE_HPP
#define MAPFILE_HPP

#include <vector>
#include <stdlib.h>

double absdiff(const double &v1, const double &v2);

class XiaData;
class TFile;

///////////////////////////////////////////////////////////////////////////////
// class MapEntry
///////////////////////////////////////////////////////////////////////////////

class MapEntry{
  public:
	unsigned int location;
	std::string type;
	std::string subtype;
	std::string tag;
	std::vector<double> args;
	
	MapEntry(){ clear(); }

	MapEntry(const std::string &input_, const char delimiter_=':'){ set(input_, delimiter_); }
	
	MapEntry(const std::string &type_, const std::string &subtype_, const std::string &tag_){ set(type_, subtype_, tag_); }
	
	MapEntry(const MapEntry &other);

	bool operator == (const MapEntry &other) const ;

	void get(std::string &type_, std::string &subtype_, std::string &tag_) const ;
	
	void set(const std::string &input_, const char delimiter_=':');
	
	void set(const std::string &type_, const std::string &subtype_, const std::string &tag_);
	
	void pushArg(const double &arg_){ args.push_back(arg_); }
	
	void clear();
	
	unsigned int increment();
	
	bool getArg(const size_t &index_, double &arg) const ;
	
	bool hasTag(const std::string &tag_) const ;

	std::string print() const ;
};

///////////////////////////////////////////////////////////////////////////////
// class MapEntryValidator
///////////////////////////////////////////////////////////////////////////////

class MapEntryValidator{
public:
	enum MapValidationModes {NO_VALIDATION=0x0, 
	                         WITH_TYPE=0x1, WITHOUT_TYPE=0x2, NO_TYPE=0x4, FORCE_TYPE=0x8,
	                         WITH_SUBTYPE=0x10, WITHOUT_SUBTYPE=0x20, NO_SUBTYPE=0x40, FORCE_SUBTYPE=0x80,
	                         WITH_TAG=0x100, WITHOUT_TAG=0x200, NO_TAG=0x400, FORCE_TAG=0x800
	                         };

	enum FailureModes {PASSED=0x0, BAD_TYPE=0x1, BAD_SUBTYPE=0x2, BAD_TAG=0x4};

	MapEntryValidator() : validation(NO_VALIDATION) { }
	
	void SetValidationMode(const int &valid){ validation = valid; }
	
	void SetValidTypes(const std::string &valid){ validTypes = valid; }
	
	void SetValidSubtypes(const std::string &valid){ validSubtypes = valid; }
	
	void SetValidTags(const std::string &valid){ validTags = valid; }
	
	void SetInvalidTypes(const std::string &invalid){ invalidTypes = invalid; }
	
	void SetInvalidSubtypes(const std::string &invalid){ invalidSubtypes = invalid; }
	
	void SetInvalidTags(const std::string &invalid){ invalidTags = invalid; }
	
	void SetValid(const std::string &types, const std::string &subtypes="", const std::string &tags="");
	
	void SetInvalid(const std::string &types, const std::string &subtypes="", const std::string &tags="");
	
	bool Validate(const MapEntry &entry) const ;
	
	bool HasBadType(const int &failcode) const { return ((failcode & BAD_TYPE) != 0); }
	
	bool HasBadSubtype(const int &failcode) const { return ((failcode & BAD_SUBTYPE) != 0); }
	
	bool HasBadTag(const int &failcode) const { return ((failcode & BAD_TAG) != 0); }
	
	void Print() const ;
	
private:
	int validation;

	std::string validTypes;
	std::string validSubtypes;
	std::string validTags;
	
	std::string invalidTypes;
	std::string invalidSubtypes;
	std::string invalidTags;
	
	bool CheckFlag(const MapValidationModes &flag) const { return ((validation & flag) != 0); }
};

///////////////////////////////////////////////////////////////////////////////
// class MapFile
///////////////////////////////////////////////////////////////////////////////

class MapFile{
  private:
  	bool init;

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
	
	MapEntry *GetMapEntry(XiaData *event_);
	
	std::vector<std::string> *GetTypes(){ return &types; }
	
	std::string GetType(int mod_, int chan_);
	
	std::string GetSubtype(int mod_, int chan_);
	
	std::string GetTag(int mod_, int chan_);
	
	void GetModChan(const int &location, int &mod, int &chan);
	
	const int GetMaxModules(){ return max_modules; }
	
	const int GetMaxChannels(){ return max_channels; }
	
	bool IsInit(){ return init; }

	void GetListOfLocations(std::vector<int> &list, const MapEntryValidator &valid);
	
	int GetFirstOccurance(const std::string &type_);
	
	int GetLastOccurance(const std::string &type_);
	
	int GetAllOccurances(const std::string &type_, std::vector<int> &locations, const bool &isSingleEnded=true);
	
	bool GetFirstStart(int &mod, int &chan);

	void ClearTypeList(){ types.clear(); }

	void AddTypeToList(const std::string &type_){ types.push_back(type_); }

	bool Load(const char *filename_);

	void PrintAllEntries();
	
	void PrintAllTypes();
	
	bool Write(TFile *f_);
};

#endif
