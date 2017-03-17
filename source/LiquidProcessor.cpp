#include "LiquidProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

#ifndef C_IN_VAC
#define C_IN_VAC 29.9792458 // cm/ns
#endif

#ifndef M_NEUTRON
#define M_NEUTRON 939.5654133 // MeV/c^2
#endif

/// Process all individual events.
bool LiquidProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (current_event->time - start->channelEvent->time)*8 + (current_event->phase - start->channelEvent->phase)*4;

	// Do time alignment.
	double r0 = 0.5;
	if(chEvt->calib->Position())
		r0 = chEvt->calib->positionCal->r0;
	
	// Do time alignment.
	if(chEvt->calib->Time()){
		chEvt->calib->timeCal->GetCalTime(tdiff);

		// Check that the adjusted time difference is reasonable.
		if(tdiff < -20 || tdiff > 200)
			return false;
	}
	
	// Get the location of this detector.
	int location = chEvt->entry->location;

	short_qdc = current_event->qdc;
	long_qdc = current_event->qdc2;

	if(histsEnabled){
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill(tdiff, location);
		loc_short_energy_2d->Fill(short_qdc, location);
		loc_long_energy_2d->Fill(long_qdc, location);
		loc_psd_2d->Fill(short_qdc/long_qdc, location);
		loc_1d->Fill(location);		
	}
	
	double energy = 0.5E4*M_NEUTRON*r0*r0/(C_IN_VAC*C_IN_VAC*tdiff*tdiff); // MeV
	
	// Fill the values into the root tree.
	structure.Append(tdiff, energy, short_qdc, long_qdc, location);
	     
	return true;
}

LiquidProcessor::LiquidProcessor(MapFile *map_) : Processor("Liquid", "liquid", map_){
	fitting_low = -7; // 28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = 7; // -28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

LiquidProcessor::~LiquidProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_short_energy_2d;
		delete loc_long_energy_2d;
		delete loc_psd_2d;
		delete loc_1d;
	}
}

void LiquidProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;
	
	int minloc = mapfile->GetFirstOccurance("liquid");
	int maxloc = mapfile->GetLastOccurance("liquid");
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_short_energy_2d = new Plotter("liquid_h2", "Liquid Location vs. S", "COLZ", "S (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_long_energy_2d = new Plotter("liquid_h3", "Liquid Location vs. L", "COLZ", "L (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc, maxloc+1);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid Location vs. PSD", "COLZ", "PSD (S/L)", 200, 0, 1, "Location", maxloc-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("liquid_h1", "Liquid Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_short_energy_2d = new Plotter("liquid_h2", "Liquid S", "COLZ", "S (a.u.)", 200, 0, 20000);
		loc_long_energy_2d = new Plotter("liquid_h3", "Liquid L", "COLZ", "L (a.u.)", 200, 0, 20000);
		loc_psd_2d = new Plotter("liquid_h4", "Liquid PSD", "COLZ", "PSD (S/L)", 200, 0, 1);
	}
	loc_1d = new Plotter("liquid_h5", "Liquid Location", "", "Location", maxloc-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_short_energy_2d);
	plots_.push_back(loc_long_energy_2d);
	plots_.push_back(loc_psd_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
