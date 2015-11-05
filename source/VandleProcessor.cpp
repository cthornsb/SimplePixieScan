#include <algorithm>
#include <cmath>

#include <iomanip>

#include "VandleProcessor.hpp"
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
	sort(events.begin(), events.end(), &ChannelEventPair::CompareChannel);
	
	std::deque<ChannelEventPair*>::iterator iter_L = events.begin();
	std::deque<ChannelEventPair*>::iterator iter_R = events.begin()+1;

	ChannelEvent *current_event_L;
	ChannelEvent *current_event_R;
	ChannelEvent *start_event = start->event;

	// Pick out pairs of channels representing vandle bars.
	for(; iter_R != events.end(); iter_L++, iter_R++){
		current_event_L = (*iter_L)->event;
		current_event_R = (*iter_R)->event;
	
		// Check that the time and energy values are valid
		if(!current_event_L->valid_chan || !current_event_R->valid_chan){ continue; }
	
		// Check that these two channels have the correct detector tag.
		if((*iter_L)->entry->tag != "left" || 
		   (*iter_R)->entry->tag != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if((current_event_L->modNum != current_event_R->modNum) || (current_event_L->chanNum+1 != current_event_R->chanNum)){ continue; }
		
		// Check that the two channels are not separated by too much time.
		if(absdiff(current_event_L->time, current_event_R->time) > (2 * small_max_tdiff)){ continue; }
		
		// Calculate the particle time-of-flight and the time difference between the two ends.		
		double tof = (current_event_L->hires_time + current_event_R->hires_time) / 2.0 - start_event->hires_time;
		//double tdiff = (current_event_L->hires_time - current_event_R->hires_time);
		
		// Fill the values into the root tree.
		structure.Append(tof, current_event_L->hires_energy, current_event_R->hires_energy, (current_event_R->hires_time - start_event->hires_time), 
		                 (current_event_R->hires_time - start_event->hires_time), std::sqrt(current_event_L->hires_energy * current_event_R->hires_energy), 
		                 (*iter_L)->entry->location);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event_L->yvals, (int*)current_event_R->yvals, current_event_L->size);
		}
		
		good_events++;
	}
	return true;
}

VandleProcessor::VandleProcessor(MapFile *map_) : Processor("Vandle", "vandle", map_){
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
