#include "Processor.hpp"

#include "ProcessorHandler.hpp"
#include "TriggerProcessor.hpp"
#include "VandleProcessor.hpp"
#include "PhoswichProcessor.hpp"
#include "LiquidBarProcessor.hpp"
#include "LiquidProcessor.hpp"
#include "HagridProcessor.hpp"
#include "GenericProcessor.hpp"
#include "LogicProcessor.hpp"

#include "MapFile.hpp"
#include "CalibFile.hpp"

XiaData *dummyEvent = new XiaData();
MapEntry dummyEntry;

ChannelEventPair dummyStart(dummyEvent, new ChannelEvent(dummyEvent), &dummyEntry, &dummyCalib);

ProcessorHandler::ProcessorHandler(){ 
	total_events = 0; 
	start_events = 0;
	first_event_time = 0.0;
	delta_event_time = 0.0;
	untriggered = false;
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

bool ProcessorHandler::ToggleTraces(){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->ToggleTraces();
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

bool ProcessorHandler::InitTraceOutput(TTree *tree_){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->ToggleTraces();
		retval = retval && iter->proc->InitializeTraces(tree_);
	}
	return true;
}

bool ProcessorHandler::CheckProcessor(std::string type_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(iter->type == type_){ return false; }
	}
	return true;
}

Processor *ProcessorHandler::AddProcessor(std::string type_, MapFile *map_){
	Processor *proc;

	if(type_ == "trigger"){ proc = (Processor*)(new TriggerProcessor(map_)); }
	else if(type_ == "vandle"){ proc = (Processor*)(new VandleProcessor(map_)); }
	else if(type_ == "phoswich"){ proc = (Processor*)(new PhoswichProcessor(map_)); }
	else if(type_ == "liquidbar"){ proc = (Processor*)(new LiquidBarProcessor(map_)); }
	else if(type_ == "liquid"){ proc = (Processor*)(new LiquidProcessor(map_)); }
	else if(type_ == "hagrid"){ proc = (Processor*)(new HagridProcessor(map_)); }
	else if(type_ == "generic"){ proc = (Processor*)(new GenericProcessor(map_)); }
	else if(type_ == "logic"){ proc = (Processor*)(new LogicProcessor(map_)); }
	else{ return NULL; }
	
	procs.push_back(ProcessorEntry(proc, type_)); 
	
	return proc;
}

bool ProcessorHandler::AddEvent(ChannelEventPair *pair_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(pair_->entry->type == iter->type){ 
			iter->proc->AddEvent(pair_); 
			if(pair_->entry->tag == "start") start_events++;
			if(total_events == 0){ first_event_time = pair_->pixieEvent->time * 8E-9; }
			delta_event_time = (pair_->pixieEvent->time * 8E-9) - first_event_time;
			total_events++; 
			return true;
		}
	}
	return false;
}

bool ProcessorHandler::AddStart(ChannelEventPair *pair_){
	if(!pair_){ return false; }
	
	starts.push_back(pair_);

	return true;
}

bool ProcessorHandler::Process(){
	// Return false if there are no start events.
	if(starts.empty()){
		if(untriggered) starts.push_back(&dummyStart);
		else return false;
	}
	
	bool retval = false;
	
	// Iterate over all start events in the starts vector.
	for(std::vector<ChannelEventPair*>::iterator start = starts.begin(); start != starts.end(); start++){
		// First call the preprocessors. The preprocessor will calculate the phase of the trace
		// by doing a CFD analysis.
		for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
			iter->proc->PreProcess();
		}

		// After preprocessing has finished, call the processors.
		for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
			if(iter->proc->Process(*start)){ retval = true; }
		}
	}
	
	return retval;
}

void ProcessorHandler::ZeroAll(){
	// Remove all pointers from the start vector.
	starts.clear();
	
	// Remove all channel events from the processors and tell the processor to finish up 
	// processing by clearing its event list. This must be done last because other processors 
	// may rely on events which are contained within this processor.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->WrapUp();
		iter->proc->Zero();
	}
}
