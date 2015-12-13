#include "TriggerProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TriggerProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->event;
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Fill the values into the root tree.
		structure.Append(current_event->time, current_event->hires_energy, current_event->phase);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

TriggerProcessor::TriggerProcessor(MapFile *map_) : Processor("Trigger", "trigger", map_){
	root_structure = (Structure*)&structure;
	root_waveform = (Waveform*)&waveform;
	
	int minloc = map_->GetFirstOccurance("trigger");
	int maxloc = map_->GetLastOccurance("trigger");

	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_energy_2d = new Plotter("trigger_h2", "Trigger Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_phase_2d = new Plotter("trigger_h3", "Trigger Location vs. Phase", "COLZ", "Phase (ns)", 100, 0, 100, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_energy_2d = new Plotter("trigger_h2", "Trigger Energy", "", "Energy (a.u.)", 200, 0, 20000);
		loc_phase_2d = new Plotter("trigger_h3", "Trigger Phase", "", "Phase (ns)", 100, 0, 100);
	}
	loc_1d = new Plotter("trigger_h4", "Trigger Location", "", "Location", maxloc-minloc, minloc, maxloc+1);
}

TriggerProcessor::~TriggerProcessor(){ 
	delete loc_energy_2d;
	delete loc_phase_2d;
	delete loc_1d;
}

void TriggerProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_phase_2d);
	plots_.push_back(loc_1d);
}
