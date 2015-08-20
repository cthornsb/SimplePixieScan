#include <algorithm>
#include <cmath>

#include <iomanip>

#include "VandleProcessor.hpp"
#include "ChannelEvent.hpp"
#include "MapFile.hpp"

#include "TTree.h"

#define C_IN_VAC 29.9792458 // cm/ns
#define C_IN_BAR 12.65822 // cm/ns

#define SMALL_LENGTH 60 // cm
#define MEDIUM_LENGTH 120 // cm
#define LARGE_LENGTH 200 // cm

const double small_max_tdiff = ((SMALL_LENGTH / C_IN_BAR) / 8E-9); // Maximum time difference between valid vandle pairwise events (pixie clock ticks)

double absdiff(const double &v1, const double &v2){
	return (v1 >= v2)?(v1-v2):(v2-v1);
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
	
	// Pick out pairs of channels representing vandle bars.
	for(; iter_R != events.end(); iter_L++, iter_R++){
		// Check that the time and energy values are valid
		if(!(*iter_L)->valid_chan || !(*iter_R)->valid_chan){ continue; }
	
		// Check that these two channels have the correct detector tag.
		if((*iter_L)->entry->tag != "left" || (*iter_R)->entry->tag != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if(((*iter_L)->modNum != (*iter_R)->modNum) || ((*iter_L)->chanNum+1 != (*iter_R)->chanNum)){ continue; }
		
		// Check that the two channels are not separated by too much time.
		if(absdiff((*iter_L)->time, (*iter_R)->time) > (2 * small_max_tdiff)){ continue; }
		
		// Calculate the particle time-of-flight and the time difference between the two ends.		
		double tof = ((*iter_L)->hires_time + (*iter_R)->hires_time) / 2.0 - start->hires_time;
		double tdiff = ((*iter_L)->hires_time - (*iter_R)->hires_time);
		
		// Fill the values into the root tree.
		structure.Append(tof, (*iter_L)->hires_energy, (*iter_R)->hires_energy, ((*iter_R)->hires_time - start->hires_time), ((*iter_R)->hires_time - start->hires_time),
		                 std::sqrt((*iter_L)->hires_energy * (*iter_R)->hires_energy), (*iter_L)->entry->location);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)(*iter_L)->yvals, (int*)(*iter_R)->yvals, (*iter_L)->size);
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
