#include "GenericProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tof;
	if(chEvt->channelEvent->traceLength != 0) // Correct for the phases of the start and the current event.
		tof = (current_event->time - start->channelEvent->time)*sysClock + (current_event->phase - start->channelEvent->phase)*adcClock;
	else
		if(start->channelEvent->traceLength != 0) // Correct for the phase of the start trace.
			tof = (current_event->time - start->channelEvent->time)*sysClock - start->channelEvent->phase*adcClock;
		else // No start trace. Cannot correct the phases.
			tof = (current_event->time - start->channelEvent->time)*sysClock;
		
	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){	
		// Fill all diagnostic histograms.
		tof_1d->Fill(location, tof);
		tqdc_1d->Fill(location, current_event->qdc);
		long_tof_2d->Fill2d(location, tof, current_event->qdc);
		maxADC_tof_2d->Fill2d(location, tof, current_event->max_ADC);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tof, current_event->qdc, location);
	
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

	online_->GenerateHist(tof_1d);
	online_->GenerateHist(tqdc_1d);
	online_->GenerateHist(long_tof_2d);
	online_->GenerateHist(maxADC_tof_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
