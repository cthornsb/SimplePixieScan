#include <algorithm>
#include <cmath>

#include <iomanip>

#include "NonwichProcessor.hpp"
#include "MapFile.hpp"

bool NonwichProcessor::HandleEvents(){
	if(!init){ return false; }
	
	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->event;
	
		// Check that this is a E-residual event. The E-loss channel should be marked as a start.
		if((*iter)->entry->tag != "E"){ continue; }
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Calculate the particle time-of-flight and the time difference between the two ends.		
		double tof = current_event->hires_time - start->event->hires_time;
		
		// Fill the values into the root tree.
		structure.Append(tof, start->event->hires_energy, current_event->hires_energy);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)start->event->yvals, (int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

NonwichProcessor::NonwichProcessor(MapFile *map_) : Processor("Nonwich", "nonwich", map_){
	root_structure = (Structure*)&structure;
	root_waveform = (Waveform*)&waveform;
}

NonwichProcessor::~NonwichProcessor(){ }
