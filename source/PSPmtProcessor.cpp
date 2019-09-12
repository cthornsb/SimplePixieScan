
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
	unsigned short tqdcIndex = 0;

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
	double tof = (channel_event->time - start->channelEvent->time)*8 + (channel_event->phase - start->channelEvent->phase)*4;

	if(isDynode) // Compute the short integral of the dynode pulse.
		channel_event->IntegratePulse2(channel_event->max_index + 5, channel_event->max_index + 50);

	unsigned short chanIdentifier = 0xFFFF;
	
	// Build up the channel identifier.
	chanIdentifier &= ~(0x000F & (channel_event->chanNum)); // Pixie channel number
	chanIdentifier &= ~(0x0010 & (isBarDet << 4));          // Is this detector part of a double-ended bar?
	chanIdentifier &= ~(0x0020 & (isRightEnd << 5));        // Is this the right side of the bar detector?
	chanIdentifier &= ~(0x0040 & (isDynode << 6));          // Is this a dynode signal?
	chanIdentifier &= ~(0x0180 & (tqdcIndex << 7));         // Index of TQDC signal (if applicable).
	//chanIdentifier &= ~(0xFE00 & (VARIABLE << 9));        // Remaining 7 bits. Currently un-used.
	chanIdentifier = ~chanIdentifier;

	if(histsEnabled){ // Fill all diagnostic histograms.
		if(!isBarDet){ // Single sided
			if(isDynode) // This is a dynode.
				evtL->addDynode(tof, channel_event->qdc, channel_event->qdc2);
			else // Only anodes get added to the tqdc spectrum. Determine which histogram to fill
				evtL->addAnode((float)channel_event->qdc, tqdcIndex);
			if(evtL->allValuesSet()){
				// Fill all histograms
				tof_1d->Fill(location, evtL->dynTime); 
				long_1d->Fill(location, evtL->dynLTQDC);
				psd_long_2d->Fill2d(location, evtL->dynLTQDC, evtL->dynSTQDC/evtL->dynLTQDC);
				long_tof_2d->Fill2d(location, evtL->dynTime, evtL->dynLTQDC);		
				xpos_1d->Fill(location, evtL->xpos);
				ypos_1d->Fill(location, evtL->ypos);
				ypos_xpos_2d->Fill2d(location, evtL->xpos, evtL->ypos);
				evtL->reset();
			}
		}
		else{ // Bar detector
			if(isDynode){ // This is a dynode.
				if(!isRightEnd) // Left side dynode
					evtL->addDynode(tof, channel_event->qdc, channel_event->qdc2);
				else // Right side dynode
					evtR->addDynode(tof, channel_event->qdc, channel_event->qdc2);
			}
			else{ // Only anodes get added to the tqdc spectrum. Determine which histogram to fill
				if(!isRightEnd) // Left side
					evtL->addAnode((float)channel_event->qdc, tqdcIndex);
				else // Right side
					evtR->addAnode((float)channel_event->qdc, tqdcIndex);
			}
			if(evtL->allValuesSet() && evtR->allValuesSet()){
				double ltqdc = std::sqrt(evtL->dynLTQDC*evtR->dynLTQDC);
				double stqdc = std::sqrt(evtL->dynSTQDC*evtR->dynSTQDC);
				double xpos = (evtL->xpos+evtR->xpos)/2;
				double ypos = (evtL->ypos+evtR->ypos)/2;				
			
				// Fill all histograms
				tdiff_1d->Fill(location, (evtL->dynTime - evtR->dynTime));
				tof_1d->Fill(location, (evtL->dynTime + evtR->dynTime)/2); // Only dynodes get added to the tof spectrum
				long_1d->Fill(location, ltqdc);
				psd_long_2d->Fill2d(location, ltqdc, stqdc/ltqdc);
				long_tof_2d->Fill2d(location, tof, ltqdc);
				xpos_1d->Fill(location, xpos);
				ypos_1d->Fill(location, ypos);
				ypos_xpos_2d->Fill2d(location, xpos, ypos);
				evtL->reset();
				evtR->reset();
			}
		}

		loc_1d->Fill(location);
	}

	// Fill the values into the root tree.
	structure.Append(tof, channel_event->qdc, channel_event->qdc2, channel_event->energy, chanIdentifier, location);

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

	online_->GenerateHist(tdiff_1d);
	online_->GenerateHist(tof_1d);
	online_->GenerateHist(long_1d);
	online_->GenerateHist(xpos_1d);
	online_->GenerateHist(ypos_1d);
	online_->GenerateHist(ypos_xpos_2d);
	online_->GenerateHist(psd_long_2d);
	online_->GenerateHist(long_tof_2d);
	online_->GenerateLocationHist(loc_1d);	

	histsEnabled = true;
}

bool PSPmtProcessor::AddDetectorLocations(std::vector<int> &locations){
	for(std::vector<PSPmtMap>::iterator detector = detMap.begin(); detector != detMap.end(); detector++){
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
