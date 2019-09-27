#include "TraceProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TraceProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tof = (current_event->time - start->channelEvent->time)*sysClock + (current_event->phase - start->channelEvent->phase)*adcClock;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){ // Fill all diagnostic histograms.
		tof_1d->Fill(location, tof);
		tqdc_1d->Fill(location, current_event->qdc);
		filter_1d->Fill(location, current_event->energy);
		maximum_1d->Fill(location, current_event->maximum);
		maxADC_1d->Fill(location, current_event->max_ADC);
		stddev_1d->Fill(location, current_event->stddev);
		baseline_1d->Fill(location, current_event->baseline);
		phase_1d->Fill(location, current_event->phase);
		clipped_1d->Fill(location, current_event->clipped);
		tqdc_tof_2d->Fill2d(location, tof, current_event->qdc);
		maxADC_tof_2d->Fill2d(location, tof, current_event->max_ADC);
		phase_phase_2d->Fill2d(location, start->channelEvent->phase, current_event->phase);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tof, current_event->phase, current_event->baseline, current_event->stddev, 
	                 current_event->maximum, current_event->qdc, current_event->energy, current_event->max_ADC, current_event->max_index, location);
	
	return true;
}

TraceProcessor::TraceProcessor(MapFile *map_) : Processor("Trace", "trace", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

void TraceProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(tof_1d);
	online_->GenerateHist(tqdc_1d);
	online_->GenerateHist(filter_1d);
	online_->GenerateHist(maximum_1d);
	online_->GenerateHist(maxADC_1d);
	online_->GenerateHist(stddev_1d);
	online_->GenerateHist(baseline_1d);
	online_->GenerateHist(phase_1d);
	online_->GenerateHist(clipped_1d);
	online_->GenerateHist(tqdc_tof_2d);
	online_->GenerateHist(maxADC_tof_2d);
	online_->GenerateHist(phase_phase_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
