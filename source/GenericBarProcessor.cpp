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
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill((tdiff_L+tdiff_R)/2, location);
		loc_energy_2d->Fill(std::sqrt(channel_event_L->qdc*channel_event_R->qdc), location);
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

GenericBarProcessor::~GenericBarProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_energy_2d;
		delete loc_1d;
	}
}

void GenericBarProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;
	
	int minloc = mapfile->GetFirstOccurance("genericbar");
	int maxloc = mapfile->GetLastOccurance("genericbar");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("genericbar_h1", "GenericBar Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("genericbar_h2", "GenericBar Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("genericbar_h1", "GenericBar Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("genericbar_h2", "GenericBar Energy", "", "Energy (a.u.)", 200, 0, 20000);
	}
	loc_1d = new Plotter("generic_h4", "GenericBar Location", "", "Location", (maxloc+1)-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
