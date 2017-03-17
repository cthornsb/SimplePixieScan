#include "HagridProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool HagridProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;

	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Do time alignment.
	if(chEvt->calib->Time()){
		chEvt->calib->timeCal->GetCalTime(tdiff);

		// Check that the adjusted time difference is reasonable.
		if(tdiff < -20 || tdiff > 200)
			return false;
	}
	
	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){ // Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_energy_2d->Fill(current_event->qdc, location);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->qdc, current_event->energy, current_event->max_ADC, location);
	
	return true;
}

HagridProcessor::HagridProcessor(MapFile *map_) : Processor("Hagrid", "hagrid", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	// Set the processor to not use the trace QDC for calibrations.
	use_integration = false;

	fitting_low = 8;
	fitting_high = 21;
}

HagridProcessor::~HagridProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_energy_2d;
		delete loc_1d;
	}
}

void HagridProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;

	int minloc = mapfile->GetFirstOccurance("hagrid");
	int maxloc = mapfile->GetLastOccurance("hagrid");

	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("hagrid_h1", "Hagrid Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("hagrid_h2", "Hagrid Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("hagrid_h1", "Hagrid Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("hagrid_h2", "Hagrid Energy", "", "Energy (a.u.)", 200, 0, 20000);
	}
	loc_1d = new Plotter("hagrid_h3", "Hagrid Location", "", "Location", maxloc-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
