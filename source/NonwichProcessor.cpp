#include "NonwichProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool NonwichProcessor::HandleEvents(){
	if(!init){ return false; }
	
	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
	
		// Check that this is a E-residual event. The E-loss channel should be marked as a start.
		if((*iter)->entry->tag != "E"){ continue; }
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Fill all diagnostic histograms.
		dE_energy_1d->Fill(start->channelEvent->hires_energy);
		E_energy_1d->Fill(current_event->hires_energy);
		tdiff_1d->Fill((current_event->event->time - start->pixieEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4);
		energy_2d->Fill(current_event->hires_energy, start->channelEvent->hires_energy);
		dE_phase_1d->Fill(start->channelEvent->phase);
		E_phase_1d->Fill(current_event->phase);
	
		// Fill the values into the root tree.
		structure.Append(start->pixieEvent->time, current_event->event->time, start->channelEvent->hires_energy, current_event->hires_energy);
		     
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)start->channelEvent->yvals, (int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

NonwichProcessor::NonwichProcessor(MapFile *map_) : Processor("Nonwich", "nonwich", map_){
	root_structure = (Structure*)&structure;
	root_waveform = (Waveform*)&waveform;
	
	dE_energy_1d = new Plotter("nonwich_h1", "Nonwich dE LR", "", "Light Response (a.u.)", 200, 0, 20000);
	E_energy_1d = new Plotter("nonwich_h2", "Nonwich E LR", "", "Light Response (a.u.)", 200, 0, 20000);
	tdiff_1d = new Plotter("nonwich_h3", "Nonwich Tdiff", "", "Tdiff (ns)", 200, -100, 100);
	energy_2d = new Plotter("nonwich_h4", "Nonwich dE vs. E", "COLZ", "E Light Response (a.u.)", 200, 0, 20000, "dE Light Response (a.u.)", 200, 0, 20000);
	dE_phase_1d = new Plotter("nonwich_h5", "Nonwich dE Phase", "", "MPV (ns)", 100, 0, 100);
	E_phase_1d = new Plotter("nonwich_h6", "Nonwich E Phase", "", "MPV (ns)", 100, 0, 100);
}

NonwichProcessor::~NonwichProcessor(){
	delete dE_energy_1d;
	delete E_energy_1d;
	delete tdiff_1d;
	delete energy_2d;
	delete dE_phase_1d;
	delete E_phase_1d;
}

void NonwichProcessor::GetHists(std::vector<Plotter*> &plots_){
	plots_.push_back(dE_energy_1d);
	plots_.push_back(E_energy_1d);
	plots_.push_back(tdiff_1d);
	plots_.push_back(energy_2d);
	plots_.push_back(dE_phase_1d);
	plots_.push_back(E_phase_1d);
}
