#include "TraceProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TraceProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->event->time - start->pixieEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->phase, current_event->baseline, current_event->stddev, 
	                 current_event->maximum, current_event->hires_energy, (int)current_event->max_index, location);
	
	// Copy the trace to the output file.
	if(write_waveform){
		waveform.Append(current_event->event->adcTrace);
	}

	return true;
}

TraceProcessor::TraceProcessor(MapFile *map_) : Processor("Trace", "trace", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}
