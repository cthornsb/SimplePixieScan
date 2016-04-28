#ifndef PROCESSOR_HANDLER_HPP
#define PROCESSOR_HANDLER_HPP

#include <vector>

class TTree;

class ChannelEventPair;
class MapEntry;
class MapFile;
class Processor;

struct ProcessorEntry{
	Processor *proc; /// Pointer to a data processor
	std::string type; /// Type of the data processor
	
	ProcessorEntry(Processor *proc_, const std::string &type_){
		proc = proc_; type = type_;
	}
};

class ProcessorHandler{
  private:
	std::vector<ProcessorEntry> procs; /// Vector of data processors

	std::vector<ChannelEventPair*> starts; /// Vector of all start events
	
	unsigned long total_events; /// Total number of start events received
	
	double first_event_time; /// Time of the first start event (in s)
	
	double delta_event_time; /// Time since the first start event (in s)

  public:
	ProcessorHandler();
	
	~ProcessorHandler();
	
	bool ToggleFitting();
	
	bool ToggleTraces();
	
	bool InitRootOutput(TTree *tree_);
	
	bool CheckProcessor(std::string type_);
	
	Processor *AddProcessor(std::string type_, MapFile *map_);
	
	bool AddEvent(ChannelEventPair *pair_);
	
	bool AddStart(ChannelEventPair *pair_);
	
	bool Process();
	
	unsigned long GetTotalEvents(){ return total_events; }
	
	double GetFirstEventTime(){ return first_event_time; }
	
	double GetDeltaEventTime(){ return delta_event_time; }
	
	void ZeroAll();
};

#endif
