#ifndef PROCESSOR_HANDLER_HPP
#define PROCESSOR_HANDLER_HPP

#include <vector>

#include "Processor.hpp"

class TTree;
class MapEntry;
class MapFile;

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
	
	unsigned long total_events; /// Total number of start events received
	
	double first_event_time; /// Time of the first start event (in s)
	
	double delta_event_time; /// Time since the first start event (in s)

  public:
	ProcessorHandler();
	
	~ProcessorHandler();
	
	bool ToggleFitting();
	
	bool InitRootOutput(TTree *tree_);
	
	bool AddProcessor(std::string type_, MapFile *map_);
	
	bool AddEvent(ChannelEvent *event_, MapEntry *entry_);
	
	bool Process(ChannelEvent *start_);
	
	unsigned long GetTotalEvents(){ return total_events; }
	
	double GetFirstEventTime(){ return first_event_time; }
	
	double GetDeltaEventTime(){ return delta_event_time; }
	
	void ZeroAll();
};

#endif
