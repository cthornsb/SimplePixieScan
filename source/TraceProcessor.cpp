#include "TraceProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TraceProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
		
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Calculate the time difference between the current event and the start.
		double tdiff = (current_event->event->time - start->pixieEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

		// Get the location of this detector.
		int location = (*iter)->entry->location;
	
		// Fill the values into the root tree.
		structure.Append(tdiff, current_event->phase, current_event->baseline, current_event->stddev, 
		                 current_event->maximum, current_event->hires_energy, (int)current_event->max_index, location);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append(current_event->event->adcTrace);
		}
		
		good_events++;
	}
	return true;
}

TraceProcessor::TraceProcessor(MapFile *map_) : Processor("Trace", "trace", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}
