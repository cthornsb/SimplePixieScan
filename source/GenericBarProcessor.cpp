#include <cmath>

#include "GenericBarProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericBarProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event_L = chEvt->channelEvent;
	ChanEvent *channel_event_R = chEvtR->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff_L = (channel_event_L->time - start->channelEvent->time)*8 + (channel_event_L->phase - start->channelEvent->phase)*4;
	double tdiff_R = (channel_event_R->time - start->channelEvent->time)*8 + (channel_event_R->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){
		double tqdc = std::sqrt(channel_event_L->qdc*channel_event_R->qdc);
		double maxADC = std::sqrt(channel_event_L->max_ADC*channel_event_R->max_ADC);
		double tdiff = (tdiff_L+tdiff_R)/2;
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_energy_2d->Fill(tqdc, location);
		tqdc_tdiff_2d->Fill(tdiff, tqdc);
		maxADC_tdiff_2d->Fill(tdiff, maxADC);
		loc_1d->Fill(location);		
	}

	// Fill the values into the root tree.
	structure.Append(tdiff_L, tdiff_R, channel_event_L->qdc, channel_event_R->qdc, location);

	return true;
}

GenericBarProcessor::GenericBarProcessor(MapFile *map_) : Processor("GenericBar", "genericbar", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &L_waveform;
	
	// Set the detector type to a bar.
	isSingleEnded = false;
}

void GenericBarProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_energy_2d);
	online_->GenerateHist(tqdc_tdiff_2d);
	online_->GenerateHist(maxADC_tdiff_2d);	
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
