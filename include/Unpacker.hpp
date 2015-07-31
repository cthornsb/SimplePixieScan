#ifndef UNPACKER_HPP
#define UNPACKER_HPP

#include <deque>
#include <vector>
#include <string>

class ChannelEvent;
class MapFile;
class ConfigFile;

std::string to_str(const int &input_);

bool is_in(const std::vector<std::string> &vec_, const std::string &input_);

class Unpacker{
  private:
	static const unsigned int TOTALREAD = 1000000;
	static const unsigned int maxWords = 131072; //Revision D
	bool full_event;

	MapFile *mapfile;
	ConfigFile *configfile;
	
	std::deque<ChannelEvent*> eventList;
	std::deque<ChannelEvent*> rawEvent;
	
	void ClearRawEvent();
	
	void ClearEventList();
	
	void DeleteCurrentEvent();

	void ProcessRawEvent();
	
	void ScanList();
	
	void SortList();
	
	int ReadBuffer(unsigned int *buf, unsigned long *bufLen);
	
  public:
	Unpacker();
	
	~Unpacker();
  
	/// Extract channel information from the raw parameter array ibuf
	bool ReadSpill(char *ibuf, unsigned int nWords, bool is_verbose=true);
};

#endif
