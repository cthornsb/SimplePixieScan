#include <cmath>

#include "LiquidBarProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

/// Process all individual events.
bool LiquidBarProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event_L = chEvt->channelEvent;
	ChanEvent *channel_event_R = chEvtR->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff_L = (channel_event_L->time - start->channelEvent->time)*8 + (channel_event_L->phase - start->channelEvent->phase)*4;
	double tdiff_R = (channel_event_R->time - start->channelEvent->time)*8 + (channel_event_R->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	short_qdc = std::sqrt(channel_event_L->qdc*channel_event_R->qdc);
	long_qdc = std::sqrt(channel_event_L->qdc2*channel_event_R->qdc2);

	if(histsEnabled){
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill((tdiff_L+tdiff_R)/2, location);
		loc_short_tqdc_2d->Fill(short_qdc, location);
		loc_long_tqdc_2d->Fill(long_qdc, location);
		loc_psd_2d->Fill(short_qdc/long_qdc, location);
		loc_1d->Fill(location);		
	}
	
	// Fill the values into the root tree.
	structure.Append(tdiff_L, tdiff_R, channel_event_L->qdc2, channel_event_R->qdc2, channel_event_L->qdc, channel_event_R->qdc, location);
	     
	return true;
}

LiquidBarProcessor::LiquidBarProcessor(MapFile *map_) : Processor("LiquidBar", "liquidbar", map_){
	fitting_low = -7; // 28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = 7; // -28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	// Set the detector type to a bar.
	isSingleEnded = false;
}

LiquidBarProcessor::~LiquidBarProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_short_tqdc_2d;
		delete loc_long_tqdc_2d;
		delete loc_psd_2d;
		delete loc_1d;
	}
}

void LiquidBarProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;
	
	int minloc = mapfile->GetFirstOccurance("liquid");
	int maxloc = mapfile->GetLastOccurance("liquid");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_short_tqdc_2d = new Plotter("liquid_h2", "Liquid Location vs. S", "COLZ", "S (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_long_tqdc_2d = new Plotter("liquid_h3", "Liquid Location vs. L", "COLZ", "L (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid Location vs. PSD", "COLZ", "PSD (S/L)", 200, 0, 1, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_short_tqdc_2d = new Plotter("liquid_h2", "Liquid S", "COLZ", "S (a.u.)", 200, 0, 20000);
		loc_long_tqdc_2d = new Plotter("liquid_h3", "Liquid L", "COLZ", "L (a.u.)", 200, 0, 20000);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid PSD", "COLZ", "PSD (S/L)", 200, 0, 1);
	}
	loc_1d = new Plotter("liquid_h5", "Liquid Location", "", "Location", maxloc-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_short_tqdc_2d);
	plots_.push_back(loc_long_tqdc_2d);
	plots_.push_back(loc_psd_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
