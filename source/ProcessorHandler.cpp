#include "ChannelEvent.hpp"

#include "ProcessorHandler.hpp"
#include "GenericProcessor.hpp"
#include "TriggerProcessor.hpp"
#include "VandleProcessor.hpp"
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

bool ProcessorHandler::SetHiResMode(bool state_/*=true*/){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->SetHiResMode(state_);
	}
	return state_;
}

bool ProcessorHandler::InitRootOutput(TTree *tree_){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->Initialize(tree_);
	}
	return true;
}

bool ProcessorHandler::AddProcessor(std::string type_, MapFile *map_){
	if(type_ == "trigger"){ 
		TriggerProcessor *proc = new TriggerProcessor(map_);
		procs.push_back(ProcessorEntry(proc, "trigger")); 
	}
	else if(type_ == "vandle"){ 
		VandleProcessor *proc = new VandleProcessor(map_);
		procs.push_back(ProcessorEntry(proc, "vandle")); 
	}
	/*else if(type_ == "liquid"){ procs.push_back(new LiquidProcessor()); }
	else if(type_ == "logic"){ procs.push_back(new LogicProcessor()); }
	else if(type_ == "ionchamber"){ procs.push_back(new IonChamberProcessor()); }
	else if(type_ == "phoswich"){ procs.push_back(new PhoswichProcessor()); }
	else if(type_ == "nonwich"){ procs.push_back(new NonwichProcessor()); }*/
	else if(type_ == "generic"){ 
		GenericProcessor *proc = new GenericProcessor(map_);
		procs.push_back(ProcessorEntry(proc, "generic")); 
	}
	else{ return false; }
	return true;
}

bool ProcessorHandler::AddEvent(ChannelEvent *event_, MapEntry *entry_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(entry_->type == iter->type){ 
			iter->proc->AddEvent(event_); 
			if(entry_->tag == "start"){ 
				if(total_events == 0){ first_event_time = event_->time * 8E-9; }
				delta_event_time = (event_->time * 8E-9) - first_event_time;
				total_events++; 
			}
			return true;
		}
	}
	return false;
}

bool ProcessorHandler::Process(ChannelEvent *start_){
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
	
	return retval;
}

void ProcessorHandler::ZeroAll(){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->Zero();
	}
}
