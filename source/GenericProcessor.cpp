#include "GenericProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff;
	if(chEvt->channelEvent->traceLength != 0) // Correct for the phases of the start and the current event.
		tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;
	else
		if(start->channelEvent->traceLength != 0) // Correct for the phase of the start trace.
			tdiff = (current_event->time - start->channelEvent->time)*8 - start->channelEvent->phase*4;
		else // No start trace. Cannot correct the phases.
			tdiff = (current_event->time - start->channelEvent->time)*8;
		
	// Do time alignment.
	if(chEvt->calib->Time()){
		chEvt->calib->timeCal->GetCalTime(tdiff);

		// Check that the adjusted time difference is reasonable.
		if(tdiff < -20 || tdiff > 200)
			return false;
	}
	
	// Get the location of this detector.
	int location = chEvt->entry->location;
	
	// Fill all diagnostic histograms.
	loc_tdiff_2d->Fill(tdiff, location);
	loc_energy_2d->Fill(current_event->qdc, location);
	loc_phase_2d->Fill(current_event->phase, location);
	loc_1d->Fill(location);

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->qdc, location);
	
	return true;
}

GenericProcessor::GenericProcessor(MapFile *map_) : Processor("Generic", "generic", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	// Do not force the use of a trace. By setting this flag to false,
	// this processor WILL NOT reject events which do not have an ADC trace.
	use_trace = false;

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
