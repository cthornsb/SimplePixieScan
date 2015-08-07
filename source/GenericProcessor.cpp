#include "GenericProcessor.hpp"
#include "ChannelEvent.hpp"
#include "MapFile.hpp"

#include "TTree.h"

bool GenericProcessor::HandleEvents(){
	if(!init){ return false; }

	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		structure.Append((*iter)->entry->location, ((*iter)->hires_time - start->hires_time), (*iter)->FindQDC());
		
		if(write_waveform){
			waveform.Append((*iter)->yvals, (*iter)->size);
		}
		
		good_events++;
	}
	return true;
}

GenericProcessor::GenericProcessor(bool write_waveform_/*=false*/, bool hires_timing_/*=false*/) : Processor("Generic", "generic", hires_timing_){
	write_waveform = write_waveform_;
}

bool GenericProcessor::Initialize(TTree *tree_){
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

void GenericProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
