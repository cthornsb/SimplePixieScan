#include "Processor.hpp"

#include "ProcessorHandler.hpp"
#include "TriggerProcessor.hpp"
#include "VandleProcessor.hpp"
#include "PhoswichProcessor.hpp"
#include "LiquidBarProcessor.hpp"
#include "LiquidProcessor.hpp"
#include "HagridProcessor.hpp"
#include "GenericProcessor.hpp"
#include "GenericBarProcessor.hpp"
#include "LogicProcessor.hpp"
#include "TraceProcessor.hpp"

#include "MapFile.hpp"
#include "CalibFile.hpp"

ChanEvent *dummyEvent = new ChanEvent();
MapEntry dummyEntry;

ChannelEventPair dummyStart(dummyEvent, &dummyEntry, &dummyCalib);

ProcessorHandler::ProcessorHandler(){ 
	total_events = 0; 
	start_events = 0;
	first_event_time = 0.0;
	delta_event_time = 0.0;
	untriggered = false;
	untrigChannel = false;
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

bool ProcessorHandler::SetPresortMode(bool state_/*=true*/){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->SetPresortMode(state_);
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
	else if(type_ == "genericbar"){ proc = (Processor*)(new GenericBarProcessor(map_)); }
	else if(type_ == "logic"){ proc = (Processor*)(new LogicProcessor(map_)); }
	else if(type_ == "trace"){ proc = (Processor*)(new TraceProcessor(map_)); }
	else{ return NULL; }
	
	procs.push_back(ProcessorEntry(proc, type_)); 
	
	return proc;
}

bool ProcessorHandler::AddEvent(ChannelEventPair *pair_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(pair_->entry->type == iter->type){ 
			iter->proc->AddEvent(pair_); 
			if(pair_->entry->hasTag("start")) start_events++;
			else if(pair_->entry->hasTag("untriggered")) untrigChannel = true;
			if(total_events == 0){ first_event_time = pair_->channelEvent->time * 8E-9; }
			delta_event_time = (pair_->channelEvent->time * 8E-9) - first_event_time;
			total_events++; 
			return true;
		}
	}
	return false;
}

bool ProcessorHandler::AddStart(ChannelEventPair *pair_){
	if(!pair_){ return false; }

	static int numStartWarnings = 0;
	
	starts.push_back(pair_);

	if(starts.size() == 2 && numStartWarnings < 10){
		double tdiff = starts.at(1)->channelEvent->time - starts.at(0)->channelEvent->time;
		std::cout << " Warning! Multiple starts in start list (tdiff = " << tdiff << " ticks). Consider decreasing event window.\n";
		if(++numStartWarnings == 10)
			std::cout << "  NOTE: Suppressing further warnings about multiple starts.\n";
	}

	return true;
}

bool ProcessorHandler::PreProcess(){
	// First call the preprocessors. The preprocessor will calculate the phase of the trace
	// by doing a CFD analysis or using the root fitting routine.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->PreProcess();
	}
	
	return true;
}

bool ProcessorHandler::Process(){
	// Call all processor preprocess routines.
	PreProcess();

	// Return false if there are no start events.
	if(starts.empty()){
		if(untriggered) starts.push_back(&dummyStart);
		else if(untrigChannel){
			starts.push_back(&dummyStart);
			for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
				iter->proc->RemoveByTag("untriggered");
			}
		}
		else return false;
	}
	
	bool retval = false;
	
	// After preprocessing has finished, call the processors.
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(iter->proc->Process(starts.front())){ retval = true; }
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

	untrigChannel = false;
}
