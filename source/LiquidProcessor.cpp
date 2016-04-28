#include "LiquidProcessor.hpp"
#include "Structures.h"
#include "MapFile.hpp"
#include "Plotter.hpp"

/// Process all individual events.
bool LiquidProcessor::HandleEvents(){
	if(!init){ return false; }
	
	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }

		// Calculate the time difference between the current event and the start.
		double tdiff = (current_event->event->time - start->pixieEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;
		
		// Get the location of this detector.
		int location = (*iter)->entry->location;

		// Compute the trace qdc of the fast and slow component of the left pmt pulse.
		short_qdc = current_event->IntegratePulse(current_event->max_index + fitting_low, current_event->max_index + fitting_high);
		long_qdc = current_event->IntegratePulse(current_event->max_index + fitting_low2, current_event->max_index + fitting_high2);	

		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_short_energy_2d->Fill(short_qdc, location);
		loc_long_energy_2d->Fill(long_qdc, location);
		loc_psd_2d->Fill(short_qdc/long_qdc, location);
		loc_1d->Fill(location);		
		
		// Fill the values into the root tree.
		structure.Append(tdiff, short_qdc, long_qdc, current_event->phase, location);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append(current_event->event->adcTrace);
		}
		
		good_events += 2;
	}
	return true;
}

LiquidProcessor::LiquidProcessor(MapFile *map_) : Processor("Liquid", "liquid", map_){
	fitting_low = 7; // 28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = -7; // -28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
	
	int minloc = map_->GetFirstOccurance("liquid");
	int maxloc = map_->GetLastOccurance("liquid");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_short_energy_2d = new Plotter("liquid_h2", "Liquid Location vs. S", "COLZ", "S (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_long_energy_2d = new Plotter("liquid_h3", "Liquid Location vs. L", "COLZ", "L (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid Location vs. PSD", "COLZ", "PSD (S/L)", 200, 0, 1, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_short_energy_2d = new Plotter("liquid_h2", "Liquid S", "COLZ", "S (a.u.)", 200, 0, 20000);
		loc_long_energy_2d = new Plotter("liquid_h3", "Liquid L", "COLZ", "L (a.u.)", 200, 0, 20000);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid PSD", "COLZ", "PSD (S/L)", 200, 0, 1);
	}
	loc_1d = new Plotter("liquid_h5", "Liquid Location", "", "Location", maxloc-minloc, minloc, maxloc+1);
}

LiquidProcessor::~LiquidProcessor(){ 
	delete loc_tdiff_2d;
	delete loc_short_energy_2d;
	delete loc_long_energy_2d;
	delete loc_psd_2d;
	delete loc_1d;
}

void LiquidProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_short_energy_2d);
	plots_.push_back(loc_long_energy_2d);
	plots_.push_back(loc_psd_2d);
	plots_.push_back(loc_1d);
}
