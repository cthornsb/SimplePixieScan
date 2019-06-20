#include "TraceProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TraceProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){ // Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(location, tdiff);
		loc_energy_2d->Fill(location, current_event->qdc);
		loc_maximum_2d->Fill(location, current_event->maximum);
		loc_phase_2d->Fill(location, current_event->phase);
		loc_baseline_2d->Fill(location, current_event->baseline);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->phase, current_event->baseline, current_event->stddev, 
	                 current_event->maximum, current_event->qdc, current_event->energy, current_event->max_ADC, current_event->max_index, location);
	
	return true;
}

TraceProcessor::TraceProcessor(MapFile *map_) : Processor("Trace", "trace", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

void TraceProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_energy_2d);
	online_->GenerateHist(loc_maximum_2d);
	online_->GenerateHist(loc_phase_2d);
	online_->GenerateHist(loc_baseline_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
