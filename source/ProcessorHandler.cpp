#include "ProcessorHandler.hpp"

#include "TriggerProcessor.hpp"

ProcessorHandler::~ProcessorHandler(){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->Status();
		delete iter->proc;
	}
}

bool ProcessorHandler::InitRootOutput(TTree *tree_){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->Initialize(tree_);
	}
	return true;
}

bool ProcessorHandler::AddProcessor(std::string type_){
	if(type_ == "trigger"){ 
		TriggerProcessor *proc = new TriggerProcessor();
		procs.push_back(ProcessorEntry(proc, "trigger")); 
	}
	/*else if(type_ == "vandle"){ procs.push_back(new VandleProcessor()); }
	else if(type_ == "liquid"){ procs.push_back(new LiquidProcessor()); }
	else if(type_ == "logic"){ procs.push_back(new LogicProcessor()); }
	else if(type_ == "ionchamber"){ procs.push_back(new IonChamberProcessor()); }
	else if(type_ == "phoswich"){ procs.push_back(new PhoswichProcessor()); }
	else if(type_ == "nonwich"){ procs.push_back(new NonwichProcessor()); }*/
	else{ return false; }
	return true;
}

bool ProcessorHandler::AddEvent(ChannelEvent* event_, std::string type_){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		if(type_ == iter->type){ 
			iter->proc->AddEvent(event_); 
			return true;
		}
	}
	return false;
}

bool ProcessorHandler::Process(ChannelEvent *start_){
	bool retval = true;
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		retval = retval && iter->proc->Process(start_);
	}
	return retval;
}

void ProcessorHandler::ZeroAll(){
	for(std::vector<ProcessorEntry>::iterator iter = procs.begin(); iter != procs.end(); iter++){
		iter->proc->Zero();
	}
}
