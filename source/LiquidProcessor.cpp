#include "LiquidProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

/// Process all individual events.
bool LiquidProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tof = (current_event->time - start->channelEvent->time)*sysClock + (current_event->phase - start->channelEvent->phase)*adcClock;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	short_qdc = current_event->qdc;
	long_qdc = current_event->qdc2;

	if(histsEnabled){
		// Fill all diagnostic histograms.
		tof_1d->Fill(location, tof);
		long_1d->Fill(location, long_qdc);
		psd_long_2d->Fill2d(location, long_qdc, short_qdc/long_qdc);
		long_tof_2d->Fill2d(location, tof, long_qdc);
		maxADC_tof_2d->Fill2d(location, tof, current_event->max_ADC);
		loc_1d->Fill(location);	
	}
	
	// Fill the values into the root tree.
	structure.Append(tof, short_qdc, long_qdc, location);
	     
	return true;
}

LiquidProcessor::LiquidProcessor(MapFile *map_) : Processor("Liquid", "liquid", map_){
	fitting_low = -7; // 28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = 7; // -28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

void LiquidProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(tof_1d);
	online_->GenerateHist(long_1d);
	online_->GenerateHist(psd_long_2d);
	online_->GenerateHist(long_tof_2d);
	online_->GenerateHist(maxADC_tof_2d);	
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
