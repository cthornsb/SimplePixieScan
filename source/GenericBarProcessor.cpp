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
		double tdiff = tdiff_R-tdiff_L;
		double tof = (tdiff_L+tdiff_R)/2;

		// Fill all diagnostic histograms.
		tdiff_1d->Fill(location, tdiff);
		tof_1d->Fill(location, tof);
		tqdc_1d->Fill(location, tqdc);
		tqdc_tof_2d->Fill2d(location, tof, tqdc);
		maxADC_tof_2d->Fill2d(location, tof, maxADC);
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

	online_->GenerateHist(tdiff_1d);
	online_->GenerateHist(tof_1d);
	online_->GenerateHist(tqdc_1d);
	online_->GenerateHist(tqdc_tof_2d);
	online_->GenerateHist(maxADC_tof_2d);	
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
