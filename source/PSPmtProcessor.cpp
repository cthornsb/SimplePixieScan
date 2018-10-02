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
	chanIdentifier &= ~(0x0010 & (isBarDet << 4));         // Is this detector part of a double-ended bar?
	chanIdentifier &= ~(0x0020 & (isRightEnd << 5));       // Is this the right side of the bar detector?
	chanIdentifier &= ~(0x00C0 & (tqdcIndex << 6));        // Index of TQDC signal (if applicable).
	//chanIdentifier &= ~(0xFF00 & (tqdcIndex << 8));        // Remaining 8 bits. Currently un-used.
	chanIdentifier = ~chanIdentifier;

	// Fill the values into the root tree.
	structure.Append(tdiff, channel_event->qdc, chanIdentifier, location);

	// In order to read back the information.
	/*location_readback =  (chanIdentifier & 0x0F);
	isBarDet_readback   = ((chanIdentifier & 0x10) != 0);
	isRightEnd_readback = ((chanIdentifier & 0x20) != 0);	
	tqdcIndex_readback  =  (chanIdentifier & 0xC0) >> 6;*/

	return true;
}

PSPmtProcessor::PSPmtProcessor(MapFile *map_) : Processor("PSPmt", "genericbar", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
	
	// Turn off diagnostic histograms.
	histsEnabled = false;
}

PSPmtProcessor::~PSPmtProcessor(){ 
}

void PSPmtProcessor::GetHists(std::vector<Plotter*> &plots_){
}
