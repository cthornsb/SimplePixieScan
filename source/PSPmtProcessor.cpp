#include <cmath>

#include "PSPmtProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

void PSPmtEvent::reset(){
	for(size_t i = 0; i < 4; i++) 
		channels[i] = false;
}

bool PSPmtEvent::addAnode(const float &anode, const size_t &index){
	if(anode > 0){
		anodes[index] = anode;
		channels[index] = true;
	}
	return allValuesSet();
}

bool PSPmtEvent::allValuesSet(){
	for(size_t i = 0; i < 4; i++)
		if(!channels[i]) return false;
	xpos = ((anodes[0]+anodes[1])-(anodes[2]+anodes[3]))/(anodes[0]+anodes[1]+anodes[2]+anodes[3]);
	ypos = ((anodes[1]+anodes[2])-(anodes[0]+anodes[3]))/(anodes[0]+anodes[1]+anodes[2]+anodes[3]);
	return true;
}

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
		if(chEvt->entry->tag.at(0) != 'V'){
			if(chEvt->entry->tag == "SE")
				tqdcIndex = 1;
			else if(chEvt->entry->tag == "NE")
				tqdcIndex = 2;
			else if(chEvt->entry->tag == "NW")
				tqdcIndex = 3;
			else if(chEvt->entry->tag == "SW")
				tqdcIndex = 4;
		}
		else{
			if(chEvt->entry->tag == "V1")
				tqdcIndex = 1;
			else if(chEvt->entry->tag == "V2")
				tqdcIndex = 2;
			else if(chEvt->entry->tag == "V3")
				tqdcIndex = 3;
			else if(chEvt->entry->tag == "V4")
				tqdcIndex = 4;
		}
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
		else{ // Only anodes get added to the tqdc spectrum. Determine which histogram to fill
			loc_energy_2d->Fill(channel_event->qdc, location);
			if(!isRightEnd){ // Left side
				if(pspmtEventL.addAnode((float)channel_event->energy, tqdcIndex-1)){
					loc_xpos_2d->Fill(pspmtEventL.xpos, location);
					loc_ypos_2d->Fill(pspmtEventL.ypos, location);
					ypos_xpos_2d->Fill(pspmtEventL.xpos, pspmtEventL.ypos);
					pspmtEventL.reset();
				}
			}
			else{ // Right side
				if(pspmtEventR.addAnode((float)channel_event->energy, tqdcIndex-1)){
					loc_xpos_2d->Fill(pspmtEventR.xpos, location);
					loc_ypos_2d->Fill(pspmtEventR.ypos, location);
					ypos_xpos_2d->Fill(pspmtEventR.xpos, pspmtEventR.ypos);
					pspmtEventR.reset();
				}
			}
		}
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

void PSPmtProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_energy_2d);
	online_->GenerateHist(loc_xpos_2d);
	online_->GenerateHist(loc_ypos_2d);
	online_->GenerateHist(ypos_xpos_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}

void PSPmtProcessor::Reset(){
	pspmtEventL.reset();
	pspmtEventR.reset();
}
