#include "GenericProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
		
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Calculate the time difference between the current event and the start.
		double tdiff = (current_event->event->time - start->pixieEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

		// Do time alignment.
		if((*iter)->calib->Time())
			tdiff += (*iter)->calib->timeCal->t0;
		
		// Get the location of this detector.
		int location = (*iter)->entry->location;
		
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_energy_2d->Fill(current_event->hires_energy, location);
		loc_phase_2d->Fill(current_event->phase, location);
		loc_1d->Fill(location);
	
		// Fill the values into the root tree.
		structure.Append(current_event->hires_energy, tdiff, location);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append(current_event->event->adcTrace);
		}
		
		good_events++;
	}
	return true;
}

GenericProcessor::GenericProcessor(MapFile *map_) : Processor("Generic", "generic", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	int minloc = map_->GetFirstOccurance("generic");
	int maxloc = map_->GetLastOccurance("generic");

	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("generic_h1", "Generic Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("generic_h2", "Generic Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_phase_2d = new Plotter("generic_h3", "Generic Location vs. Phase", "COLZ", "Phase (ns)", 100, 0, 100, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("generic_h1", "Generic Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("generic_h2", "Generic Energy", "", "Energy (a.u.)", 200, 0, 20000);
		loc_phase_2d = new Plotter("generic_h3", "Generic Phase", "", "Phase (ns)", 100, 0, 100);
	}
	loc_1d = new Plotter("generic_h4", "Generic Location", "", "Location", maxloc-minloc, minloc, maxloc+1);
}

GenericProcessor::~GenericProcessor(){ 
	delete loc_tdiff_2d;
	delete loc_energy_2d;
	delete loc_phase_2d;
	delete loc_1d;
}

void GenericProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_phase_2d);
	plots_.push_back(loc_1d);
}
