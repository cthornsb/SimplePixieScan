#include "TraceProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TraceProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){ // Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_energy_2d->Fill(current_event->qdc, location);
		loc_maximum_2d->Fill(current_event->maximum, location);
		loc_phase_2d->Fill(current_event->phase, location);
		loc_baseline_2d->Fill(current_event->baseline, location);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, current_event->phase, current_event->baseline, current_event->stddev, 
	                 current_event->maximum, current_event->qdc, current_event->energy, current_event->max_ADC, current_event->max_index, location);
	
	return true;
}

TraceProcessor::TraceProcessor(MapFile *map_) : Processor("Trace", "trace", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

TraceProcessor::~TraceProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_energy_2d;
		delete loc_maximum_2d;
		delete loc_phase_2d;
		delete loc_baseline_2d;
		delete loc_1d;
	}
}

void TraceProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;

	int minloc = mapfile->GetFirstOccurance("trace");
	int maxloc = mapfile->GetLastOccurance("trace");

	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("trace_h1", "Trace Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("trace_h2", "Trace Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_maximum_2d = new Plotter("trace_h3", "Trace Location vs. Peak Max", "COLZ", "Maximum (ADC)", 200, 0, 5000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_phase_2d = new Plotter("trace_h4", "Trace Location vs. Phase", "COLZ", "Phase (ns)", 200, 0, 1000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_baseline_2d = new Plotter("trace_h5", "Trace Location vs. Baseline", "COLZ", "Baseline (ADC)", 200, 0, 1000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("trace_h1", "Trace Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("trace_h2", "Trace Energy", "", "Energy (a.u.)", 200, 0, 20000);
		loc_maximum_2d = new Plotter("trace_h3", "Trace Peak Max", "", "Maximum (ADC)", 200, 0, 5000);
		loc_phase_2d = new Plotter("trace_h4", "Trace Phase", "", "Phase (ns)", 200, 0, 1000);
		loc_baseline_2d = new Plotter("trace_h5", "Trace Baseline", "", "Baseline (ADC)", 200, 0, 1000);
	}
	loc_1d = new Plotter("trace_h3", "Trace Location", "", "Location", (maxloc+1)-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_maximum_2d);
	plots_.push_back(loc_phase_2d);
	plots_.push_back(loc_baseline_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
