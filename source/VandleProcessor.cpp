#include <algorithm>
#include <cmath>

#include "VandleProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

#ifndef C_IN_VAC
#define C_IN_VAC 29.9792458 // cm/ns
#endif
#define C_IN_VANDLE_BAR 13.2354 // cm/ns (13.2354 +/- 1.09219) CRT Dec. 16th, 2015 bar 1022)

#ifndef M_NEUTRON
#define M_NEUTRON 939.5654133 // MeV/c^2
#endif

#define VANDLE_BAR_LENGTH 60 // cm

const double max_tdiff = ((VANDLE_BAR_LENGTH / C_IN_VANDLE_BAR) / 8E-9); // Maximum time difference between valid vandle pairwise events (pixie clock ticks)

bool VandleProcessor::HandleEvents(){
	if(!init || events.size() <= 1){ 
		return false;
	}
	
	// Sort the vandle event list by channel ID. This way, we will be able
	// to determine which channels are neighbors and, thus, part of the
	// same vandle bar.
	sort(events.begin(), events.end(), &ChannelEventPair::CompareChannel);
	
	std::deque<ChannelEventPair*>::iterator iter_L = events.begin();
	std::deque<ChannelEventPair*>::iterator iter_R = events.begin()+1;

	XiaData *current_event_L;
	XiaData *current_event_R;

	ChannelEvent *channel_event_L;
	ChannelEvent *channel_event_R;

	// Pick out pairs of channels representing vandle bars.
	for(; iter_R != events.end(); iter_L++, iter_R++){
		current_event_L = (*iter_L)->pixieEvent;
		current_event_R = (*iter_R)->pixieEvent;

		channel_event_L = (*iter_L)->channelEvent;
		channel_event_R = (*iter_R)->channelEvent;
	
		// Check that the time and energy values are valid
		if(!channel_event_L->valid_chan || !channel_event_R->valid_chan){ continue; }
	
		// Check that these two channels have the correct detector tag.
		if((*iter_L)->entry->subtype != "left" || 
		   (*iter_R)->entry->subtype != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if((current_event_L->modNum != current_event_R->modNum) || (current_event_L->chanNum+1 != current_event_R->chanNum)){ continue; }
		
		// Check that the two channels are not separated by too much time.
		if(absdiff(current_event_L->time, current_event_R->time) > (2 * max_tdiff)){ continue; }

		// Calculate the time difference between the current event and the start.
		double tdiff_L = (current_event_L->time - start->pixieEvent->time)*8 + (channel_event_L->phase - start->channelEvent->phase)*4;
		double tdiff_R = (current_event_R->time - start->pixieEvent->time)*8 + (channel_event_R->phase - start->channelEvent->phase)*4;

		// Do time alignment.
		double r0 = 0.5;
		if((*iter_L)->calib->Position())
			r0 = (*iter_L)->calib->positionCal->r0;
		
		if((*iter_L)->calib->Time() && (*iter_R)->calib->Time()){
			(*iter_L)->calib->timeCal->GetCalTime(tdiff_L);
			(*iter_R)->calib->timeCal->GetCalTime(tdiff_R);
		}
		
		// Get the location of this detector.
		int location = (*iter_L)->entry->location;
		
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill((tdiff_L + tdiff_R)/2.0, location);
		loc_energy_2d->Fill(std::sqrt(channel_event_L->hires_energy*channel_event_R->hires_energy), location);
		loc_L_phase_2d->Fill(channel_event_L->phase, location);
		loc_R_phase_2d->Fill(channel_event_R->phase, location);
		loc_1d->Fill(location);		
		
		double ypos = (tdiff_R - tdiff_L)*C_IN_VANDLE_BAR/200.0; // m
		double tof = (tdiff_L + tdiff_R)/2.0; // ns
		double ctof = (r0/std::sqrt(r0*r0+ypos*ypos))*tof; // ns
		double energy = 0.5E4*M_NEUTRON*r0*r0/(C_IN_VAC*C_IN_VAC*ctof*ctof); // MeV
		
		// Fill the values into the root tree.
		structure.Append(std::sqrt(channel_event_L->hires_energy*channel_event_R->hires_energy), ypos, ctof, energy, location);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			L_waveform.Append(channel_event_L->event->adcTrace);
			R_waveform.Append(channel_event_R->event->adcTrace);
		}
		
		good_events += 2;
	}
	return true;
}

VandleProcessor::VandleProcessor(MapFile *map_) : Processor("Vandle", "vandle", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &L_waveform;
	
	int minloc = map_->GetFirstOccurance("vandle");
	int maxloc = map_->GetLastOccurance("vandle");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("vandle_h1", "Vandle Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("vandle_h2", "Vandle Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_L_phase_2d = new Plotter("vandle_h3", "Vandle Location vs. L Phase", "COLZ", "Phase (ns)", 100, 0, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_R_phase_2d = new Plotter("vandle_h4", "Vandle Location vs. R Phase", "COLZ", "Phase (ns)", 100, 0, 100, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("vandle_h1", "Vandle Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("vandle_h2", "Vandle Energy", "", "Energy (a.u.)", 200, 0, 20000);
		loc_L_phase_2d = new Plotter("vandle_h3", "Vandle L Phase", "", "Phase (ns)", 100, 0, 100);
		loc_R_phase_2d = new Plotter("vandle_h4", "Vandle R Phase", "", "Phase (ns)", 100, 0, 100);
	}
	loc_1d = new Plotter("vandle_h5", "Vandle Location", "", "Location", maxloc-minloc, minloc, maxloc+1);
}

VandleProcessor::~VandleProcessor(){ 
	delete loc_tdiff_2d;
	delete loc_energy_2d;
	delete loc_L_phase_2d;
	delete loc_R_phase_2d;
	delete loc_1d;
}

void VandleProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_L_phase_2d);
	plots_.push_back(loc_R_phase_2d);
	plots_.push_back(loc_1d);
}
