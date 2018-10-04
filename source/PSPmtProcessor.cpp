#include <cmath>

#include "PSPmtProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool PSPmtProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event = chEvt->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (channel_event->time - start->channelEvent->time)*8 + (channel_event->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	unsigned short location = chEvt->entry->location;

	unsigned short chanIdentifier = 0xFFFF;
	
	bool isBarDet = false;
	bool isRightEnd = false;
	unsigned short tqdcIndex = 0;
	
	// Check if this is a bar type detector
	if(!chEvt->entry->subtype.empty()){
		if(chEvt->entry->subtype == "right")
			isRightEnd = true;
	}
		
	// Check for anode tags
	if(!chEvt->entry->tag.empty()){
		if(chEvt->entry->tag == "SE")
			tqdcIndex = 1;
		else if(chEvt->entry->tag == "NE")
			tqdcIndex = 2;
		else if(chEvt->entry->tag == "NW")
			tqdcIndex = 3;
		else if(chEvt->entry->tag == "SW")
			tqdcIndex = 4;
	}
	
	// Build up the channel identifier.
	chanIdentifier &= ~(0x000F & (channel_event->chanNum)); // Pixie module channel number
	chanIdentifier &= ~(0x0010 & (isBarDet << 4));          // Is this detector part of a double-ended bar?
	chanIdentifier &= ~(0x0020 & (isRightEnd << 5));        // Is this the right side of the bar detector?
	chanIdentifier &= ~(0x00C0 & (tqdcIndex << 6));         // Index of TQDC signal (if applicable).
	//chanIdentifier &= ~(0xFF00 & (tqdcIndex << 8));        // Remaining 8 bits. Currently un-used.
	chanIdentifier = ~chanIdentifier;

	if(histsEnabled){ // Fill all diagnostic histograms.
		if(tqdcIndex == 0) // Only dynodes get added to the tdiff spectrum
			loc_tdiff_2d->Fill(tdiff, location);
		else // Only anodes get added to the tqdc spectrum
			loc_energy_2d->Fill(channel_event->qdc, location);
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, channel_event->qdc, channel_event->qdc2, chanIdentifier, location);

	// In order to read back the information.
	/*location_readback =  (chanIdentifier & 0x0F);
	isBarDet_readback   = ((chanIdentifier & 0x10) != 0);
	isRightEnd_readback = ((chanIdentifier & 0x20) != 0);	
	tqdcIndex_readback  =  (chanIdentifier & 0xC0) >> 6;*/

	return true;
}

PSPmtProcessor::PSPmtProcessor(MapFile *map_) : Processor("PSPmt", "pspmt", map_){
	fitting_low = -7; // 28 ns
	fitting_high = 50; // 200 ns
	fitting_low2 = 7; // -28 ns
	fitting_high2 = 50; // 200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

PSPmtProcessor::~PSPmtProcessor(){ 
	if(histsEnabled){
		delete loc_tdiff_2d;
		delete loc_energy_2d;
		delete loc_1d;
	}
}

void PSPmtProcessor::GetHists(std::vector<Plotter*> &plots_){
	if(histsEnabled) return;

	int minloc = mapfile->GetFirstOccurance("pspmt");
	int maxloc = mapfile->GetLastOccurance("pspmt");

	if(maxloc-minloc > 1){ // More than one detector. Define 2d plots.
		loc_tdiff_2d = new Plotter("pspmt_h1", "PSPMT Location vs. Tdiff", "COLZ", "Tdiff (ns)", 200, -100, 100, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
		loc_energy_2d = new Plotter("pspmt_h2", "PSPMT Location vs. Energy", "COLZ", "Energy (a.u.)", 200, 0, 20000, "Location", (maxloc+1)-minloc, minloc, maxloc+1);
	}
	else{ // Only one detector. Define 1d plots instead.
		loc_tdiff_2d = new Plotter("pspmt_h1", "PSPMT Tdiff", "", "Tdiff (ns)", 200, -100, 100);
		loc_energy_2d = new Plotter("pspmt_h2", "PSPMT Energy", "", "Energy (a.u.)", 200, 0, 20000);
	}
	loc_1d = new Plotter("pspmt_h3", "PSPMT Location", "", "Location", (maxloc+1)-minloc, minloc, maxloc+1);

	plots_.push_back(loc_tdiff_2d);
	plots_.push_back(loc_energy_2d);
	plots_.push_back(loc_1d);

	histsEnabled = true;
}