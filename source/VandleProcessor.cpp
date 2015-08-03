#include <algorithm>
#include <cmath>

#include "VandleProcessor.hpp"
#include "ChannelEvent.hpp"
#include "MapFile.hpp"

#include "TTree.h"

#define C_IN_VAC 29.9792458 // cm/ns
#define C_IN_BAR 12.65822 // cm/ns

#define SMALL_LENGTH 60 // cm
#define MEDIUM_LENGTH 120 // cm
#define LARGE_LENGTH 200 // cm

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
		
		//double tdiff = (*iter_L)->time - (*iter_R)->time;
		//double y_hit = tdiff * C_IN_BAR; // cm
		/*double left_hires_time = phase * adcClockInSeconds + chan->GetTrigTime() * filterClockInSeconds;
		double right_hires_time = phase * adcClockInSeconds + chan->GetTrigTime() * filterClockInSeconds;
		double timediff = left_hires_time - right_hires_time;*/
		
		double tof = ((*iter_L)->time + (*iter_R)->time) / 2.0;
		
		structure.Append((*iter_L)->entry->location/2, tof, (*iter_L)->energy, (*iter_R)->energy, std::sqrt((*iter_L)->energy * (*iter_R)->energy));
		
		if(write_waveform){
			waveform.Append((*iter_L)->trace, (*iter_R)->trace);
		}
		
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
		local_branch = tree_->Branch((type+"Wave").c_str(), &waveform);
	}
	
	return (init = true);
}

void VandleProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
