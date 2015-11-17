#include "TriggerProcessor.hpp"

bool TriggerProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->event;
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Fill the values into the root tree.
		structure.Append(current_event->hires_time, current_event->hires_energy);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

TriggerProcessor::TriggerProcessor(MapFile *map_) : Processor("Trigger", "trigger", map_){
	root_structure = (Structure*)&structure;
	root_waveform = (Waveform*)&waveform;
}

TriggerProcessor::~TriggerProcessor(){ }
