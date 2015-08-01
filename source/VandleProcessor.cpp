#include <algorithm>
#include <cmath>

#include "VandleProcessor.hpp"
#include "ChannelEvent.hpp"

#include "TTree.h"

bool VandleProcessor::HandleEvents(){
	if(!init || events.size() <= 1){ 
		return false;
	}
	
	// Sort the vandle event list by channel ID. This way, we will be able
	// to determine which channels are neighbors and, thus, part of the
	// same vandle bar.
	sort(events.begin(), events.end(), &ChannelEvent::CompareChannel);
	
	std::deque<ChannelEvent*>::iterator iter_L = events.begin();
	std::deque<ChannelEvent*>::iterator iter_R = events.begin()+1;
	
	// Pick out pairs of channels representing vandle bars
	for(; iter_R != events.end(); iter_L++, iter_R++){
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if(iter_R != events.end() && ((*iter_L)->modNum != (*iter_R)->modNum) && ((*iter_L)->chanNum-1 != (*iter_R)->chanNum)){ continue; }
		
		//std::cout << (*iter_L)->modNum << ", " << (*iter_L)->chanNum << ", " << (*iter_R)->modNum << ", " << (*iter_R)->chanNum << std::endl;
		//std::cout << " difftime = " << (*iter_L)->time - (*iter_R)->time << std::endl;
		//std::cout << " qdc = " << std::sqrt((*iter_L)->energy*(*iter_R)->energy) << std::endl;
		/*structure.Append((*iter)->time, (*iter)->energy);
		
		if(write_waveform){
			left_waveform.Append((*iter)->trace);
		}*/
		
		good_events += 2;
	}
	return true;
}

VandleProcessor::VandleProcessor(bool write_waveform_/*=false*/) : Processor("Vandle", "vandle"){
	write_waveform = write_waveform_;
}

bool VandleProcessor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(type.c_str(), &structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((type+"WaveL").c_str(), &left_waveform);
		local_branch = tree_->Branch((type+"WaveR").c_str(), &right_waveform);
	}
	
	return (init = true);
}

void VandleProcessor::Zero(){
	structure.Zero();
	left_waveform.Zero();
	right_waveform.Zero();
}
