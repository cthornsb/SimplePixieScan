#include "GenericProcessor.hpp"
#include "MapFile.hpp"

#include "TTree.h"

bool GenericProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->event;
		
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Fill the values into the root tree.
		structure.Append((current_event->hires_time - start->event->hires_time), current_event->hires_energy, (*iter)->entry->location);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

GenericProcessor::GenericProcessor(MapFile *map_) : Processor("Generic", "generic", map_){
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
