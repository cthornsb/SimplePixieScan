#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <deque>
#include <vector>
#include <string>

#include "Unpacker.hpp"
#include "Structures.h"

class MapFile;
class ConfigFile;
class ProcessorHandler;

class Scanner : public Unpacker{
  private:
	MapFile *mapfile;
	ConfigFile *configfile;
	ProcessorHandler *handler;
	
	RawEventStructure structure;
	
	/// Process all events in the event list.
	void ProcessRawEvent();
	
	/** Initialize the map file, the config file, the processor handler, and add
	 * all of the required processors.
	 */
	bool Initialize();
	
  public:
	Scanner();
	
	Scanner(std::string fname_, bool overwrite_=true, bool debug_mode_=false);
	
	~Scanner();

	bool InitRootOutput(std::string fname_, bool overwrite_=true);

	bool SetHiResMode(bool state_=true);
};

#endif
