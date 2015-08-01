#ifndef PROCESSOR_HANDLER_HPP
#define PROCESSOR_HANDLER_HPP

#include <vector>

#include "Processor.hpp"

class TTree;

struct ProcessorEntry{
	Processor *proc;
	std::string type;
	
	ProcessorEntry(Processor *proc_, const std::string &type_){
		proc = proc_; type = type_;
	}
};

class ProcessorHandler{
  private:
	std::vector<ProcessorEntry> procs;
	
	unsigned long total_events;

  public:
	ProcessorHandler(){ total_events = 0; }
	
	~ProcessorHandler();
	
	bool InitRootOutput(TTree *tree_);
	
	bool AddProcessor(std::string type_);
	
	bool AddEvent(ChannelEvent* event_, std::string type_);
	
	bool Process(ChannelEvent *start_);
	
	void ZeroAll();
};

#endif
