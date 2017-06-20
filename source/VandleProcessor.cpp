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
const double max_ctof = (1/C_IN_VAC)*std::sqrt(1E5*M_NEUTRON); // Set the minimum neutron energy to 50 keV.

bool VandleProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event_L = chEvt->channelEvent;
	ChanEvent *channel_event_R = chEvtR->channelEvent;
	
	// Check that the two channels are not separated by too much time.
	if(absdiff(channel_event_L->time, channel_event_R->time) > (2 * max_tdiff)){ return false; }

	// Calculate the time difference between the current event and the start.
	double tdiff_L = (channel_event_L->time - start->channelEvent->time)*8 + (channel_event_L->phase - start->channelEvent->phase)*4;
	double tdiff_R = (channel_event_R->time - start->channelEvent->time)*8 + (channel_event_R->phase - start->channelEvent->phase)*4;

	// Get the detector distance from the target and the detector angle with respect to the beam axis.
	double r0 = 0.5, theta0 = 0.0;
	if(chEvt->calib->Position()){
		r0 = chEvt->calib->positionCal->r0;
		theta0 = addAngles(chEvt->calib->positionCal->theta, std::atan(drand(-0.015, 0.015)/r0));
	}
	
	double radius=r0, theta=0.0, phi=0.0, ypos=0.0, ctof=0.0;
	if(chEvt->calib->Time() && chEvtR->calib->Time()){ // Do time alignment.
		chEvt->calib->timeCal->GetCalTime(tdiff_L);
		chEvtR->calib->timeCal->GetCalTime(tdiff_R);

		radius = std::sqrt(r0*r0 + ypos*ypos);
		ctof = (r0/radius)*(tdiff_L + tdiff_R)/2.0; // ns

		// Check that the corrected neutron ToF is reasonable.
		if(ctof < -20 || ctof > r0*max_ctof) return false;

		ypos = (tdiff_R - tdiff_L)*C_IN_VANDLE_BAR/200.0; // m
		theta = std::acos(std::cos(theta0)/std::sqrt(1.0+ypos*ypos/(r0*r0)));
		phi = std::atan2(ypos, r0*std::sin(theta0));
	}
	else{ // No time alignment available.
		ypos = 0.0;
		ctof = (tdiff_L + tdiff_R)/2.0; // ns
	}

	double energy = 0.5E4*M_NEUTRON*r0*r0/(C_IN_VAC*C_IN_VAC*ctof*ctof); // MeV
	
	// Get the location of this detector.
	int location = chEvt->entry->location;

	if(histsEnabled){	
		// Fill all diagnostic histograms.
		loc_tdiff_2d->Fill((tdiff_L + tdiff_R)/2.0, location);
		loc_energy_2d->Fill(std::sqrt(channel_event_L->qdc*channel_event_R->qdc), location);
		loc_L_phase_2d->Fill(channel_event_L->phase, location);
		loc_R_phase_2d->Fill(channel_event_R->phase, location);
		loc_1d->Fill(location);	
	}
	
	// Fill the values into the root tree.
	structure.Append(ctof, radius, theta*rad2deg, phi*rad2deg, energy, std::sqrt(channel_event_L->qdc*channel_event_R->qdc), location);
	     
	return true;
}

VandleProcessor::VandleProcessor(MapFile *map_) : Processor("Vandle", "vandle", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &L_waveform;
	
	// Set the detector type to a bar.
	isSingleEnded = false;
}

VandleProcessor::~VandleProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_energy_2d;
		delete loc_L_phase_2d;
		delete loc_R_phase_2d;
		delete loc_1d;
	}
}

void VandleProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;

	int minloc = mapfile->GetFirstOccurance("vandle");
	int maxloc = mapfile->GetLastOccurance("vandle");
	
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


	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_L_phase_2d);
	plots_.push_back(loc_R_phase_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}
