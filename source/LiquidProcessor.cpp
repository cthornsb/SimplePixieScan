#include "LiquidProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

/// Process all individual events.
bool LiquidProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	short_qdc = current_event->qdc;
	long_qdc = current_event->qdc2;

	if(histsEnabled){
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_short_tqdc_2d->Fill(short_qdc, location);
		loc_long_tqdc_2d->Fill(long_qdc, location);
		loc_psd_2d->Fill(short_qdc/long_qdc, location);
		loc_1d->Fill(location);		
	}
	
	// Fill the values into the root tree.
	structure.Append(tdiff, short_qdc, long_qdc, location);
	     
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

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_short_tqdc_2d);
	online_->GenerateHist(loc_long_tqdc_2d);
	online_->GenerateHist(loc_psd_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}
