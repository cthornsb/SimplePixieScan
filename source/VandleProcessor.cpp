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

double GetWalk(float val){
	if(val < 175){ return (1.09099 * std::log(val) - 7.76641); }
	else if(val > 3700){ return (0.0); }
	else{ return ((-9.13743E-12) * std::pow(val, 3.0) + (1.9485e-7) * pow(val, 2.0) - 0.000163286 * val - 2.13918); }
}

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
		// Check that these two channels have the correct detector tag
		if((*iter_L)->entry->tag != "left" || (*iter_R)->entry->tag != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if(((*iter_L)->modNum != (*iter_R)->modNum) || ((*iter_L)->chanNum+1 != (*iter_R)->chanNum)){ continue; }
		
		double left_hires_time = (*iter_L)->hires_time;// - GetWalk((*iter_L)->baseline);
		double right_hires_time = (*iter_R)->hires_time;// - GetWalk((*iter_R)->baseline);
		double start_hires_time = start->hires_time;// - GetWalk(start->baseline);
		
		double tof = (left_hires_time + right_hires_time) / 2.0 - start_hires_time;
		
		structure.Append((*iter_L)->entry->location, tof, (*iter_L)->FindQDC(), (*iter_R)->FindQDC(), (left_hires_time - start->hires_time), 
		                 (right_hires_time - start->hires_time), std::sqrt((*iter_L)->FindQDC() * (*iter_R)->FindQDC()));
		                 
		if(write_waveform){
			waveform.Append((*iter_L)->yvals, (*iter_R)->yvals, (*iter_L)->size);
		}
		
		good_events++;
	}
	return true;
}

VandleProcessor::VandleProcessor(bool write_waveform_/*=false*/, bool hires_timing_/*=false*/) : Processor("Vandle", "vandle", hires_timing_){
	write_waveform = write_waveform_;
}

bool VandleProcessor::Initialize(TTree *tree_){
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

void VandleProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
