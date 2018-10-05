#include "GenericProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff;
	if(chEvt->channelEvent->traceLength != 0) // Correct for the phases of the start and the current event.
		tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;
	else
		if(start->channelEvent->traceLength != 0) // Correct for the phase of the start trace.
			tdiff = (current_event->time - start->channelEvent->time)*8 - start->channelEvent->phase*4;
		else // No start trace. Cannot correct the phases.
			tdiff = (current_event->time - start->channelEvent->time)*8;
		
	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){	
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_energy_2d->Fill(current_event->qdc, location);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->qdc, location);
	
	return true;
}

GenericProcessor::GenericProcessor(MapFile *map_) : Processor("Generic", "generic", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	// Do not force the use of a trace. By setting this flag to false,
	// this processor WILL NOT reject events which do not have an ADC trace.
	use_trace = false;
}

void GenericProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_energy_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
