#ifndef MAPFILE_HPP
#define MAPFILE_HPP

#include <vector>
#include <stdlib.h>

/** Convert all uppercase letters in a string to lowercase
  */
void lowercaseString(std::string &str);

class XiaData;
class TFile;

/** @class MapEntry
  * @brief Container for an entry in the simpleScan channel map
  * @author Cory R. Thornsberry (cthornsb@vols.utk.edu)
  * @date September 20, 2019
  */

class MapEntry{
public:
	unsigned int location; ///< The Pixie ID of the entry (e.g. mod*16+chan for Pixie-16)
	std::string type; ///< The type name of the detector
	std::string subtype; ///< The subtype name of the detector
	std::string tag; ///< Detector tags
	std::vector<double> args; ///< Vector of additional numerical arguments for the detector entry
	
	/** Default constructor (ignored detector entry)
	  */
	MapEntry(){ clear(); }

	/** Colon-delimited string constructor
	  */
	MapEntry(const std::string &input_, const char delimiter_=':'){ set(input_, delimiter_); }
	
	/** Explicit constructor
	  */
	MapEntry(const std::string &type_, const std::string &subtype_, const std::string &tag_){ set(type_, subtype_, tag_); }
	
	/** Copy constructor
	  */
	MapEntry(const MapEntry &other);

	/** Equality operator
	  */
	bool operator == (const MapEntry &other) const ;

	/** Get the type, subtype, and tags for this detector entry
	  */
	void get(std::string &type_, std::string &subtype_, std::string &tag_) const ;
	
	/** Set the type, subtype, and tags using a colon-delimited string
	  */
	void set(const std::string &input_, const char delimiter_=':');
	
	/** Set the type, subtype, and tags explicitly
	  */
	void set(const std::string &type_, const std::string &subtype_, const std::string &tag_);
	
	/** Push a numerical value onto the list of additional arguments
	  */
	void pushArg(const double &arg_){ args.push_back(arg_); }
	
	/** Clear the entry and set it to "ignore" type
	  */
	void clear();
	
	/** Increment the Pixie ID of this entry
	  */
	unsigned int increment();
	
	/** Get a numerical value from the list of additional parameters
	  * @return True if the requested index exists, and return false otherwise
	  */ 
	bool getArg(const size_t &index_, double &arg) const ;
	
	/** Return true if the specified tag is contained within this entry's tag string and return false otherwise
	  */
	bool hasTag(const std::string &tag_) const ;

	/** Print infomration about the map entry
	  */
	std::string print() const ;
};

/** @class MapEntryValidator
  * @brief Allows for complex testing of simpleScan map entries to check if they meet certain criteria
  * @author Cory R. Thornsberry (cthornsb@vols.utk.edu)
  * @date September 20, 2019
  */

class MapEntryValidator{
public:
	/** Map entry validation modes. Individual modes may be ORed together to generate
	  * arbitrarily complex validation modes
	  */
	enum MapValidationModes {NO_VALIDATION=0x0,    ///< Do no validation, all entries will be considered valid
	                         WITH_TYPE=0x1,        ///< Entry must have a type in the list of valid types
	                         WITHOUT_TYPE=0x2,     ///< Entry must not have a type in the list of invalid types
	                         NO_TYPE=0x4,          ///< Entry must not have a type
	                         FORCE_TYPE=0x8,       ///< Entry must have a type
	                         WITH_SUBTYPE=0x10,    ///< Entry must have a subtype in the list of valid subtypes
	                         WITHOUT_SUBTYPE=0x20, ///< Entry must not have a subtype in the list of invalid subtypes
	                         NO_SUBTYPE=0x40,      ///< Entry must not have a subtype
	                         FORCE_SUBTYPE=0x80,   ///< Entry must have a subtype
	                         WITH_TAG=0x100,       ///< Entry must have a tag in the list of valid tags
	                         WITHOUT_TAG=0x200,    ///< Entry must not have a tag in the list of invalid tags
	                         NO_TAG=0x400,         ///< Entry must not have a tag
	                         FORCE_TAG=0x800       ///< Entry must have a tag
	};

	/** Default constructor
	  */
	MapEntryValidator() : validation(NO_VALIDATION) { }
	
	/** Set the map entry validation mode. Complex validation modes may be set by ORing various
	  * MapValidationModes flags together
	  */
	void SetValidationMode(const int &valid){ validation = valid; }
	
	/** Set the list of valid detector type names
	  * @param types Space-delimited list of valid type names used when the @a WITH_TYPE bit is set
	  */
	void SetValidTypes(const std::string &valid){ validTypes = valid; }

	/** Set the list of valid detector subtype names
	  * @param types Space-delimited list of valid subtype names used when the @a WITH_SUBTYPE bit is set
	  */
	void SetValidSubtypes(const std::string &valid){ validSubtypes = valid; }
	
	/** Set the list of valid detector tags
	  * @param types Space-delimited list of valid tags used when the @a WITH_TAG bit is set
	  */
	void SetValidTags(const std::string &valid){ validTags = valid; }
	
	/** Set the list of invalid detector type names
	  * @param types Space-delimited list of invalid type names used when the @a WITHOUT_TYPE bit is set
	  */
	void SetInvalidTypes(const std::string &invalid){ invalidTypes = invalid; }
	
	/** Set the list of invalid detector subtype names
	  * @param types Space-delimited list of invalid subtype names used when the @a WITHOUT_SUBTYPE bit is set
	  */
	void SetInvalidSubtypes(const std::string &invalid){ invalidSubtypes = invalid; }

	/** Set the list of invalid detector tags
	  * @param types Space-delimited list of invalid tags used when the @a WITHOUT_TAG bit is set
	  */	
	void SetInvalidTags(const std::string &invalid){ invalidTags = invalid; }

	/** Set the list of valid detector type names, subtype names, and tags
	  * @param types Space-delimited list of valid type names used when the @a WITH_TYPE bit is set
	  * @param subtypes Space-delimited list of valid subtype names used when the @a WITH_SUBTYPE bit is set
	  * @param tags Space-delimited list of valid tags used when the @a WITH_TAG bit is set
	  */	
	void SetValid(const std::string &types, const std::string &subtypes="", const std::string &tags="");
	
	/** Set the list of invalid detector type names, subtype names, and tags
	  * @param types Space-delimited list of invalid type names used when the @a WITHOUT_TYPE bit is set
	  * @param subtypes Space-delimited list of invalid subtype names used when the @a WITHOUT_TYPE bit is set
	  * @param tags Space-delimited list of invalid tags used when the @a WITHOUT_TYPE bit is set
	  */
	void SetInvalid(const std::string &types, const std::string &subtypes="", const std::string &tags="");

	/** Validate an input map entry
	  * @param entry The map entry to check for validity
	  * @return 0 if the input map entry is valid and a positive number if it is invalid. In the event of failed
	  *         entry validation, the return value will have its MapValidationModes bits set according to which 
	  *         tests were failed. These bits may be tested using CheckMode(const int &, const MapValidationModes &)
	  */
	int Validate(const MapEntry &entry) const ;

	/** Check if a validation mode bit is set
	  * @return True if a given validation mode bit is set, and return false otherwise
	  */
	bool CheckMode(const MapValidationModes &bit) const { return CheckMode(validation, bit); }

	/** Check if a validation mode bit is set
	  * @return True if a given validation mode bit is set, and return false otherwise
	  */	
	bool CheckMode(const int &mode, const MapValidationModes &bit) const { return ((mode & bit) != 0); }
	
	/** Reset the validation mode
	  */
	void Reset(){ validation = NO_VALIDATION; }
	
	/** Print information about the validator
	  */
	void Print() const ;
	
	/** Print all the failure bits which are set in the return value of Validate()
	  */
	void PrintFailure(const int &failcode) const ;
	
private:
	int validation; ///< The validation mode to use to check the validity of map entries

	std::string validTypes; ///< The list of valid detector type names
	std::string validSubtypes; ///< The list of valid detector subtype names
	std::string validTags; ///< The list of valid detector tags
	
	std::string invalidTypes; ///< The list of invalid detector type names
	std::string invalidSubtypes; ///< The list of invalid detector subtype names
	std::string invalidTags; ///< The list of invalid detector tags
};

/** @class MapFile
  * @brief Detector channel map file handler
  * @author Cory R. Thornsberry (cthornsb@vols.utk.edu)
  * @date September 20, 2019
  */

class MapFile{
private:
  	bool init; ///< Flag indicating that the map has been properly initialized

	static const int max_modules = 14; ///< The maximum number of digitizer modules in the system
	static const int max_channels = 16; ///< The maximum number of channels in a single digitizer module
  
	MapEntry detectors[max_modules][max_channels]; ///< Matrix of all individual channel map entries
	std::vector<std::string> types; ///< Vector of defined detector types
	int max_defined_module; ///< Index of the highest module with defined entries
	
	/** Clear all detector entries in the map
	  */
	void clearEntries();

	/** Parse a colon-delimited input string specifying a range of values. The input string must have the format left:right[char]
	  * where @a left and @a right are integer values, and the right side specifier may have an optional additional character which 
	  * will be returned as parameter @a leftover
	  * @param input Input range string having the format left:right[char]
	  * @param left String of numerical characters to the left of the colon delimiter
	  * @param right String of numerical characters to the right of the colon delimiter
	  * @param leftover The non-numerical character immediately following the right-most numerical value in the input string (if any)
	  * @return True if the input string has the correct format (i.e. both the @a left and @a right integers are read correctly) and
	  *         return false otherwise
	  */
	bool parseString(const std::string &input, int &left, int &right, char &leftover) const ;
	
public:
	/** Default constructor
	  */
	MapFile();
  
  	/** Constructor which will read from an input map file
  	  */
	MapFile(const char *filename_);

	/** Destructor
	  */
	~MapFile(){ }

	/** Get the index of the maximum defined module in the map
	  */
	int GetMaxModule() const { return max_defined_module; }

	/** Get a pointer to a map entry at a specified module and channel or a NULL pointer if the entry is not found
	  */
	MapEntry *GetMapEntry(int mod_, int chan_);

	/** Get a pointer to a map entry corresponding to a XiaData event or a NULL pointer if the entry is not found
	  */	
	MapEntry *GetMapEntry(XiaData *event_);
	
	/** Get a pointer to the vector of all unique detector types
	  */
	std::vector<std::string> *GetTypes(){ return &types; }
	
	/** Get the type of the detector entry at a specified module and channel or an empty string if the entry is not found
	  */
	std::string GetType(int mod_, int chan_) const ;

	/** Get the subtype of the detector entry at a specified module and channel or an empty string if the entry is not found
	  */	
	std::string GetSubtype(int mod_, int chan_) const ;
	
	/** Get the tag of the detector entry at a specified module and channel or an empty string if the entry is not found
	  */
	std::string GetTag(int mod_, int chan_) const ;

	/** Get the corresponding digitizer module and channel for a given channel ID
	  */	
	void GetModChan(const int &location, int &mod, int &chan) const ;

	/** Get the maximum number of digitizer module in the system
	  */
	const int GetMaxModules() const { return max_modules; }
	
	/** Get the maximum number of channels per digitizer module
	  */
	const int GetMaxChannels() const { return max_channels; }

	/** Return true if the map has been successfully read from the input file, and return false otherwise
	  */	
	bool IsInit() const { return init; }

	/** Get a list of detector entries matching a certain criteria 
	  * @param list Vector of all channel IDs which match the search criteria
	  * @param valid The validator to use to check for matching detector entries (see MapEntryValidator class for more information)
	  * @return The channel ID of the first occurance of the specified entry type, or -1 if the type was not found
	  */	
	void GetListOfLocations(std::vector<int> &list, const MapEntryValidator &valid) const ;

	/** Get the first occurance of a specified type of detector entry
	  * @param type_ The type of detector entry to search for
	  * @return The channel ID of the first occurance of the specified entry type, or -1 if the type was not found
	  */	
	int GetFirstOccurance(const std::string &type_) const ;

	/** Get the last occurance of a specified type of detector entry
	  * @param type_ The type of detector entry to search for
	  * @return The channel ID of the last occurance of the specified entry type, or -1 if the type was not found
	  */	
	int GetLastOccurance(const std::string &type_) const ;

	/** Get all occurances of a specified type of detector entry
	  * @param type_ The type of detector entry to search for
	  * @param locations Vector of all channel IDs of the specified type
	  * @param isSingleEnded Flag indicating that both even and odd IDs will be considered. If set to false, odd IDs will be ignored
	  * @return The number of occurances of the specified type
	  */
	int GetAllOccurances(const std::string &type_, std::vector<int> &locations, const bool &isSingleEnded=true) const ;

	/** Get the first instance of a detector entry which has the tag "start"
	  * @return True if an entry with tag "start" is found in the map and return false otherwise
	  */
	bool GetFirstStart(int &mod, int &chan) const ;

	/** Clear the list of unique detector entry types which were defined in the map file
	  */
	void ClearTypeList(){ types.clear(); }

	/** Add a detector entry type to the list of defined types
	  */
	void AddTypeToList(const std::string &type_){ types.push_back(type_); }

	bool Load(const char *filename_);

	/** Print all detector entries for all channels
	  */
	void PrintAllEntries() const ;

	/** Print all unique detector entry types which were defined in the map file
	  */
	void PrintAllTypes() const ;
	
	/**
	  */
	bool Write(TFile *f_) const ;
};

#endif
