#include "TriggerProcessor.hpp"
#include "ChannelEvent.hpp"

#include "TTree.h"

bool TriggerProcessor::HandleEvents(){
	if(!init){ return false; }

	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		// Check that the time and energy values are valid
		if(!(*iter)->valid_chan){ continue; }
	
		// Fill the values into the root tree.
		structure.Append((*iter)->hires_time, (*iter)->hires_energy);
		
		if(write_waveform){
			waveform.Append((*iter)->yvals, (*iter)->size);
		}
		
		good_events++;
	}
	return true;
}

TriggerProcessor::TriggerProcessor(bool write_waveform_/*=false*/, bool hires_timing_/*=false*/) : Processor("Trigger", "trigger", hires_timing_){
	write_waveform = write_waveform_;
}

bool TriggerProcessor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(name.c_str(), &structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((name+"Wave").c_str(), &waveform);
	}
	
	return (init = true);
}

void TriggerProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
