#include "HagridProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool HagridProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;

	// Calculate the time difference between the current event and the start.
	double tof = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){ // Fill all diagnostic histograms.
		tof_1d->Fill(location, tof);
		tqdc_1d->Fill(location, current_event->qdc);
		filter_1d->Fill(location, current_event->energy);
		maxADC_1d->Fill(location, current_event->max_ADC);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tof, current_event->qdc, current_event->energy, current_event->max_ADC, location);
	
	return true;
}

HagridProcessor::HagridProcessor(MapFile *map_) : Processor("Hagrid", "hagrid", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	fitting_low = 8;
	fitting_high = 21;
}

void HagridProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(tof_1d);
	online_->GenerateHist(tqdc_1d);
	online_->GenerateHist(filter_1d);
	online_->GenerateHist(maxADC_1d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
