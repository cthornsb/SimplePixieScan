#include "TriggerProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TriggerProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }

		// Fill all diagnostic histograms.
		energy_1d->Fill(current_event->hires_energy);
		phase_1d->Fill(current_event->phase);
	
		// Fill the values into the root tree.
		structure.Append(current_event->event->time, current_event->hires_energy, current_event->phase);
		
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
	
	energy_1d = new Plotter("trigger_h1", "Trigger Energy", "", "Energy (a.u.)", 200, 0, 20000);
	phase_1d = new Plotter("trigger_h2", "Trigger Phase", "", "Phase (ns)", 100, 0, 100);
}

TriggerProcessor::~TriggerProcessor(){ 
	delete energy_1d;
	delete phase_1d;
}

void TriggerProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(energy_1d);
	plots_.push_back(phase_1d);
}
