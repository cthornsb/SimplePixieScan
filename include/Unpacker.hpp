#ifndef UNPACKER_HPP
#define UNPACKER_HPP

#include <deque>
#include <vector>
#include <string>

#include "Structures.h"

class ChannelEvent;
class MapFile;
class ConfigFile;
class ProcessorHandler;

class TFile;
class TTree;

std::string to_str(const int &input_);

bool is_in(const std::vector<std::string> &vec_, const std::string &input_);

class Unpacker{
  private:
	static const unsigned int TOTALREAD = 1000000;
	static const unsigned int maxWords = 131072; //Revision D
	bool full_event;
	bool debug_mode;
	bool raw_event_mode;
	bool init;

	MapFile *mapfile;
	ConfigFile *configfile;
	ProcessorHandler *handler;
	
	std::deque<ChannelEvent*> eventList;
	std::deque<ChannelEvent*> rawEvent;
	
	RawEventStructure structure;
	
	TFile *root_file;
	TTree *root_tree;
	
	void ClearRawEvent();
	
	void ClearEventList();
	
	void DeleteCurrentEvent();

	void ProcessRawEvent();
	
	void ScanList();
	
	void SortList();
	
	int ReadBuffer(unsigned int *buf, unsigned long *bufLen);
	
  public:
	Unpacker();
	
	Unpacker(std::string fname_, bool overwrite_=true, bool debug_mode_=false);
	
	~Unpacker();

	bool Initialize();

	bool InitRootOutput(std::string fname_, bool overwrite_=true);
	
	bool IsInit(){ return init; }

	bool SetDebugMode(bool state_=true){ return (debug_mode = state_); }
	
	bool SetHiResMode(bool state_=true);

	bool SetRawEventMode(bool state_=true){ return (raw_event_mode = state_); }

	/// Extract channel information from the raw parameter array ibuf
	bool ReadSpill(unsigned int *data, unsigned int nWords, bool is_verbose=true);
};

#endif
