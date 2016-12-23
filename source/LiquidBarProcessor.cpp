#include <cmath>

#include "LiquidBarProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

#ifndef C_IN_VAC
#define C_IN_VAC 29.9792458 // cm/ns
#endif
#define C_IN_LIQUID_BAR 8.845 // cm/ns (8.845 +/- 2.197) CRT Apr. 9th, 2016 bar no. 001 (mr. Napth))

#ifndef M_NEUTRON
#define M_NEUTRON 939.5654133 // MeV/c^2
#endif

#define LIQUID_BAR_LENGTH 27.94 // cm

const double max_tdiff = ((LIQUID_BAR_LENGTH / C_IN_LIQUID_BAR) / 8E-9); // Maximum time difference between valid vandle pairwise events (pixie clock ticks)

/// Process all individual events.
bool LiquidBarProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
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

		// Check that the adjusted time differences are reasonable.
		if((tdiff_L < -20 || tdiff_L > 200) || (tdiff_R < -20 || tdiff_R > 200))
			return false;

		ypos = (tdiff_R - tdiff_L)*C_IN_LIQUID_BAR/200.0; // m
		radius = std::sqrt(r0*r0 + ypos*ypos);
		theta = std::acos(std::cos(theta0)/std::sqrt(1.0+ypos*ypos/(r0*r0)));
		phi = std::atan2(ypos, r0*std::sin(theta0));
		ctof = (r0/radius)*(tdiff_L + tdiff_R)/2.0; // ns
	}
	else{ // No time alignment available.
		ypos = 0.0;
		ctof = (tdiff_L + tdiff_R)/2.0; // ns
	}

	double energy = 0.5E4*M_NEUTRON*r0*r0/(C_IN_VAC*C_IN_VAC*ctof*ctof); // MeV
	
	// Get the location of this detector.
	int location = chEvt->entry->location;

	// Compute the trace qdc of the fast component of the left and right pmt pulses.
	double stqdc = (double)std::sqrt(channel_event_L->qdc*channel_event_R->qdc);

	// Compute the trace qdc of the slow component of the left and right pmt pulses.
	double ltqdc = (double)std::sqrt(channel_event_L->qdc2*channel_event_R->qdc2);
	
	// Fill all diagnostic histograms.
	loc_tdiff_2d->Fill((tdiff_L + tdiff_R)/2.0, location/2);
	loc_short_energy_2d->Fill(stqdc, location/2);
	loc_long_energy_2d->Fill(ltqdc, location/2);
	loc_psd_2d->Fill(stqdc/ltqdc, location/2);
	loc_1d->Fill(location/2);		

	// Fill the values into the root tree.
	structure.Append(stqdc, ltqdc, radius, theta*rad2deg, phi*rad2deg, ctof, energy, location);
	     
	return true;
}

LiquidBarProcessor::LiquidBarProcessor(MapFile *map_) : Processor("LiquidBar", "liquidbar", map_){
	fitting_low = -7; // -28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = 7; // 28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &L_waveform;
	
	int minloc = map_->GetFirstOccurance("liquidbar");
	int maxloc = map_->GetLastOccurance("liquidbar");

	// Set the detector type to a bar.
	isSingleEnded = false;
	
	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("liquidbar_h1", "Liquid Bar Location vs. Avg. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_short_energy_2d = new Plotter("liquidbar_h2", "Liquid Bar Location vs. S", "COLZ", "S (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_long_energy_2d = new Plotter("liquidbar_h3", "Liquid Bar Location vs. L", "COLZ", "L (a.u.)", 200, 0, 20000, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
		loc_psd_2d = new Plotter("liquidbar_h4", "Liquid Bar Location vs. PSD", "COLZ", "PSD (S/L)", 200, 0, 1, "Location", maxloc-minloc, minloc/2, (maxloc+1)/2);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("liquidbar_h1", "Liquid Bar Avg. Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_short_energy_2d = new Plotter("liquidbar_h2", "Liquid Bar S", "COLZ", "S (a.u.)", 200, 0, 20000);
		loc_long_energy_2d = new Plotter("liquidbar_h3", "Liquid Bar L", "COLZ", "L (a.u.)", 200, 0, 20000);
		loc_psd_2d = new Plotter("liquidbar_h4", "Liquid Bar PSD", "COLZ", "PSD (S/L)", 200, 0, 1);
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
