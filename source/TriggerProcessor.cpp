#include "TriggerProcessor.hpp"
#include "ChannelEvent.hpp"

#include "TTree.h"

bool TriggerProcessor::HandleEvents(){
	if(!init){ return false; }

	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		structure.Append((*iter)->time, (*iter)->energy);
		
		if(write_waveform){
			waveform.Append((*iter)->trace);
		}
		
		good_events++;
	}
	return true;
}

TriggerProcessor::TriggerProcessor(bool write_waveform_/*=false*/) : Processor("Trigger", "trigger"){
	write_waveform = write_waveform_;
}

bool TriggerProcessor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(type.c_str(), &structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((type+"Wave").c_str(), &waveform);
	}
	
	return (init = true);
}

void TriggerProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
