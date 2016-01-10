#include <algorithm>

#include "LiquidBarProcessor.hpp"
#include "Structures.h"
#include "MapFile.hpp"
#include "Plotter.hpp"

#ifndef C_IN_VAC
#define C_IN_VAC 29.9792458 // cm/ns
#endif
#define C_IN_LIQUID_BAR 13.2354 // cm/ns (13.2354 +/- 1.09219) CRT Dec. 16th, 2015 bar 1022)

#define LIQUID_BAR_LENGTH 30.48 // cm

const double max_tdiff = ((LIQUID_BAR_LENGTH / C_IN_LIQUID_BAR) / 8E-9); // Maximum time difference between valid vandle pairwise events (pixie clock ticks)

/// Process all individual events.
bool LiquidBarProcessor::HandleEvents(){
	if(!init || events.size() <= 1){ 
		return false;
	}
	
	// Sort the liquid event list by channel ID. This way, we will be able
	// to determine which channels are neighbors and, thus, part of the
	// same liquid bar.
	sort(events.begin(), events.end(), &ChannelEventPair::CompareChannel);
	
	std::deque<ChannelEventPair*>::iterator iter_L = events.begin();
	std::deque<ChannelEventPair*>::iterator iter_R = events.begin()+1;

	ChannelEvent *current_event_L;
	ChannelEvent *current_event_R;

	// Pick out pairs of channels representing liquid bars.
	for(; iter_R != events.end(); iter_L++, iter_R++){
		current_event_L = (*iter_L)->event;
		current_event_R = (*iter_R)->event;
	
		// Check that the time and energy values are valid
		if(!current_event_L->valid_chan || !current_event_R->valid_chan){ continue; }
	
		// Check that these two channels have the correct detector tag.
		if((*iter_L)->entry->tag != "left" || 
		   (*iter_R)->entry->tag != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if((current_event_L->modNum != current_event_R->modNum) || (current_event_L->chanNum+1 != current_event_R->chanNum)){ continue; }
		
		// Check that the two channels are not separated by too much time.
		if(absdiff(current_event_L->time, current_event_R->time) > (2 * max_tdiff)){ continue; }

		// Calculate the time difference between the current event and the start.
		double tdiff_L = (current_event_L->time - start->event->time)*8 + (current_event_L->phase - start->event->phase)*4;
		double tdiff_R = (current_event_R->time - start->event->time)*8 + (current_event_R->phase - start->event->phase)*4;
		
		// Get the location of this detector.
		int location = (*iter_L)->entry->location;

		// Compute the trace qdc of the fast and slow component of the left pmt pulse.
		left_short_qdc = current_event_L->IntegratePulse(current_event_L->max_index - fitting_low, current_event_L->max_index + fitting_high);
		left_long_qdc = current_event_L->IntegratePulse(current_event_L->max_index + fitting_low2, current_event_L->max_index + fitting_high2);	

		// Compute the trace qdc of the fast and slow component of the right pmt pulse.
		right_short_qdc = current_event_R->IntegratePulse(current_event_R->max_index - fitting_low, current_event_R->max_index + fitting_high);
		right_long_qdc = current_event_R->IntegratePulse(current_event_R->max_index + fitting_low2, current_event_R->max_index + fitting_high2);	
		
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill((tdiff_L + tdiff_R)/2.0, location/2);
		loc_short_energy_2d->Fill((left_short_qdc + right_short_qdc)/2.0, location/2);
		loc_long_energy_2d->Fill((left_long_qdc + right_long_qdc)/2.0, location/2);
		loc_psd_2d->Fill((left_short_qdc + right_short_qdc)/(left_long_qdc + right_long_qdc), location/2);
		loc_1d->Fill(location/2);		
		
		// Fill the values into the root tree.
		structure.Append(tdiff_L, tdiff_R, left_short_qdc, left_long_qdc, right_short_qdc, right_long_qdc, current_event_L->phase, current_event_R->phase, location);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event_L->yvals, (int*)current_event_R->yvals, current_event_L->size);
		}
		
		good_events += 2;
	}
	return true;
}

LiquidBarProcessor::LiquidBarProcessor(MapFile *map_) : Processor("LiquidBar", "liquidbar", map_){
	fitting_low = 5;
	fitting_high = 8;
	fitting_low2 = 15;
	fitting_high2 = 100;

	root_structure = (Structure*)&structure;
	root_waveform = (Waveform*)&waveform;
	
	int minloc = map_->GetFirstOccurance("liquidbar");
	int maxloc = map_->GetLastOccurance("liquidbar");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("liquidbar_h1", "Liquid Bar Location vs. Avg. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_short_energy_2d = new Plotter("liquidbar_h2", "Liquid Bar Location vs. Avg. S", "COLZ", "S (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_long_energy_2d = new Plotter("liquidbar_h3", "Liquid Bar Location vs. Avg. L", "COLZ", "L (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_psd_2d = new Plotter("liquidbar_h4", "Liquid Bar Location vs. Avg. PSD", "COLZ", "PSD (S/L)", 200, 0, 1, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("liquidbar_h1", "Liquid Bar Avg. Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_short_energy_2d = new Plotter("liquidbar_h2", "Liquid Bar Avg. S", "COLZ", "S (a.u.)", 200, 0, 20000);
		loc_long_energy_2d = new Plotter("liquidbar_h3", "Liquid Bar Avg. L", "COLZ", "L (a.u.)", 200, 0, 20000);
		loc_psd_2d = new Plotter("liquidbar_h4", "Liquid Bar Avg. PSD", "COLZ", "PSD (S/L)", 200, 0, 1);
	}
	loc_1d = new Plotter("liquidbar_h5", "Liquid Bar Location", "", "Location", maxloc-minloc, minloc, maxloc+1);
}

LiquidBarProcessor::~LiquidBarProcessor(){ 
	delete loc_tdiff_2d;
	delete loc_short_energy_2d;
	delete loc_long_energy_2d;
	delete loc_psd_2d;
	delete loc_1d;
}

void LiquidBarProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_short_energy_2d);
	plots_.push_back(loc_long_energy_2d);
	plots_.push_back(loc_psd_2d);
	plots_.push_back(loc_1d);
}
