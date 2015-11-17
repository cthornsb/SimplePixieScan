#include "Processor.hpp"

#include "ProcessorHandler.hpp"
#include "TriggerProcessor.hpp"
#include "VandleProcessor.hpp"
#include "PhoswichProcessor.hpp"
#include "NonwichProcessor.hpp"
#include "GenericProcessor.hpp"
#include "LogicProcessor.hpp"

#include "MapFile.hpp"

ProcessorHandler::ProcessorHandler(){ 
	total_events = 0; 
	first_event_time = 0.0;
	delta_event_time = 0.0;
}

ProcessorHandler::~ProcessorHandler(){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->Status(total_events);
		delete iter->proc;
	}
}

bool ProcessorHandler::ToggleFitting(){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->ToggleFitting();
	}
	return retval;
}

bool ProcessorHandler::InitRootOutput(TTree *tree_){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->Initialize(tree_);
	}
	return true;
}

bool ProcessorHandler::CheckProcessor(std::string type_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(iter->type == type_){ return false; }
	}
	return true;
}

bool ProcessorHandler::AddProcessor(std::string type_, MapFile *map_){
	Processor *proc;

	if(type_ == "trigger"){ proc = (Processor*)(new TriggerProcessor(map_)); }
	else if(type_ == "vandle"){ proc = (Processor*)(new VandleProcessor(map_)); }
	else if(type_ == "phoswich"){ proc = (Processor*)(new PhoswichProcessor(map_)); }
	else if(type_ == "nonwich"){ proc = (Processor*)(new NonwichProcessor(map_)); }
	else if(type_ == "generic"){ proc = (Processor*)(new GenericProcessor(map_)); }
	else if(type_ == "logic"){ proc = (Processor*)(new LogicProcessor(map_)); }
	else{ return false; }
	
	procs.push_back(ProcessorEntry(proc, type_)); 
	
	return true;
}

bool ProcessorHandler::AddEvent(ChannelEventPair *pair_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(pair_->entry->type == iter->type){ 
			iter->proc->AddEvent(pair_); 
			if(pair_->entry->tag == "start"){ 
				if(total_events == 0){ first_event_time = pair_->event->time * 8E-9; }
				delta_event_time = (pair_->event->time * 8E-9) - first_event_time;
				total_events++; 
			}
			return true;
		}
	}
	return false;
}

bool ProcessorHandler::Process(ChannelEventPair *start_){
	bool retval = false;
	
	// First call the preprocessors. The preprocessor will calculate the phase of the trace
	// by doing a CFD analysis.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->PreProcess();
	}
	
	// After preprocessing has finished, call the processors.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(iter->proc->Process(start_)){ retval = true; }
	}
	
	// Finally, tell the processor to finish up processing by clearing its event list. This
	// must be done last because other processors may rely on events which are contained
	// within this processor.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->WrapUp();
	}
	
	return retval;
}

void ProcessorHandler::ZeroAll(){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->Zero();
	}
}
