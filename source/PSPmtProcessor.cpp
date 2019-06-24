
#include "PSPmtProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

///////////////////////////////////////////////////////////////////////////////
// class PSPmtProcessor
///////////////////////////////////////////////////////////////////////////////

bool PSPmtProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event = chEvt->channelEvent;

	// Get the location of this detector.
	unsigned short location = chEvt->entry->location;

	bool isDynode;
	bool isBarDet;
	bool isRightEnd;
	unsigned short tqdcIndex;

	PSPmtEvent *evtL, *evtR;

	// Find the detector associated with this channel
	bool foundMatch = false;
	for(std::vector<PSPmtMap>::iterator detector = detMap.begin(); detector != detMap.end(); detector++){
		if(detector->check(location, isDynode, isRightEnd, tqdcIndex)){
			isBarDet = detector->getDoubleSided();
			evtL = detector->getEventL();
			evtR = detector->getEventR();
			location = detector->getLocation();
			foundMatch = true;
			break;
		}
	}
	
	if(!foundMatch)
		return false;
	
	// Calculate the time difference between the current event and the start.
	double tdiff = (channel_event->time - start->channelEvent->time)*8 + (channel_event->phase - start->channelEvent->phase)*4;

	if(isDynode) // Compute the short integral of the dynode pulse.
		channel_event->IntegratePulse2(channel_event->max_index + 5, channel_event->max_index + 50);

	unsigned short chanIdentifier = 0xFFFF;
	
	// Build up the channel identifier.
	chanIdentifier &= ~(0x000F & (channel_event->chanNum)); // Pixie module channel number
	chanIdentifier &= ~(0x0010 & (isBarDet << 4));          // Is this detector part of a double-ended bar?
	chanIdentifier &= ~(0x0020 & (isRightEnd << 5));        // Is this the right side of the bar detector?
	chanIdentifier &= ~(0x00C0 & (tqdcIndex << 6));         // Index of TQDC signal (if applicable).
	//chanIdentifier &= ~(0xFF00 & (tqdcIndex << 8));        // Remaining 8 bits. Currently un-used.
	chanIdentifier = ~chanIdentifier;

	if(histsEnabled){ // Fill all diagnostic histograms.
		if(tqdcIndex == 0){ // This is a dynode.
			loc_tdiff_2d->Fill(location, tdiff); // Only dynodes get added to the tdiff spectrum
			sltqdc_ltqdc_2d->Fill2d(location, channel_event->qdc, channel_event->qdc2/channel_event->qdc);
		}
		else{ // Only anodes get added to the tqdc spectrum. Determine which histogram to fill
			loc_energy_2d->Fill(location, channel_event->qdc);
			if(!isRightEnd){ // Left side
				if(evtL->addAnode((float)channel_event->qdc, tqdcIndex-1)){
					loc_xpos_2d->Fill(location, evtL->xpos);
					loc_ypos_2d->Fill(location, evtL->ypos);
					ypos_xpos_2d->Fill2d(location, evtL->xpos, evtL->ypos);
					evtL->reset();
				}
			}
			else{ // Right side
				if(evtR->addAnode((float)channel_event->qdc, tqdcIndex-1)){
					loc_xpos_2d->Fill(location, evtR->xpos);
					loc_ypos_2d->Fill(location, evtR->ypos);
					ypos_xpos_2d->Fill2d(location, evtR->xpos, evtR->ypos);
					evtR->reset();
				}
			}
		}
		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tdiff, channel_event->qdc, channel_event->qdc2, channel_event->energy, chanIdentifier, location);

	// In order to read back the information.
	/*location_readback =  (chanIdentifier & 0x0F);
	isBarDet_readback   = ((chanIdentifier & 0x10) != 0);
	isRightEnd_readback = ((chanIdentifier & 0x20) != 0);	
	tqdcIndex_readback  =  (chanIdentifier & 0xC0) >> 6;*/

	return true;
}

PSPmtProcessor::PSPmtProcessor(MapFile *map_) : Processor("PSPmt", "pspmt", map_){
	fitting_low = 7; // Max-28 ns
	fitting_high = 50; // Max+200 ns

	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	// Read the map
	PSPmtMap::readMapFile(map_, detMap);

	// Do not force the use of a trace. By setting this flag to false,
	// this processor WILL NOT reject events which do not have an ADC trace.
	use_trace = false;
}

void PSPmtProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(loc_tdiff_2d);
	online_->GenerateHist(loc_energy_2d);
	online_->GenerateHist(loc_xpos_2d);
	online_->GenerateHist(loc_ypos_2d);
	online_->GenerateHist(ypos_xpos_2d);
	online_->GenerateHist(sltqdc_ltqdc_2d);
	online_->GenerateLocationHist(loc_1d);

	histsEnabled = true;
}

bool PSPmtProcessor::AddDetectorLocations(std::vector<int> &locations){
	for(std::vector<PSPmtMap>::iterator detector = detMap.begin(); detector != detMap.end(); detector++){
		std::cout << "debug: " << detector->getLocation() << std::endl;
		locations.push_back(detector->getLocation());
	}	
	return true; 
}

void PSPmtProcessor::Reset(){
	for(std::vector<PSPmtMap>::iterator detector = detMap.begin(); detector != detMap.end(); detector++){
		detector->getEventL()->reset();
		detector->getEventR()->reset();
	}
}
