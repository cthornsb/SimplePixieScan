#include <cmath>
#include <algorithm>

#include "PSPmtProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

unsigned short getIndexFromTag(const std::string &tag){
	unsigned short retval = 0;
	if(tag.at(0) != 'V'){
		if(tag == "SE")
			retval = 1;
		else if(tag == "NE")
			retval = 2;
		else if(tag == "NW")
			retval = 3;
		else if(tag == "SW")
			retval = 4;
	}
	else{
		if(tag == "V1")
			retval = 1;
		else if(tag == "V2")
			retval = 2;
		else if(tag == "V3")
			retval = 3;
		else if(tag == "V4")
			retval = 4;
	}
	return retval;
}

bool compareTime(const PSPmtMap &lhs, const PSPmtMap &rhs){ 
	return (lhs.getLocation() < rhs.getLocation()); 
}

///////////////////////////////////////////////////////////////////////////////
// class PSPmtMap
///////////////////////////////////////////////////////////////////////////////

PSPmtMap::PSPmtMap() : isDoubleSided(false), index1(0), index2(0) {
	for(size_t i = 0; i < 2; i++){
		for(size_t j = 0; j < 5; j++){
			channels[i][j] = -1;
		}
	}
}

PSPmtMap::PSPmtMap(const int &dynode) : PSPmtMap() { 
	channels[0][0] = dynode;
}

PSPmtMap::PSPmtMap(const int &dynodeL, const int &dynodeR) : PSPmtMap() {
	channels[0][0] = dynodeL;
	channels[1][0] = dynodeR;
	isDoubleSided = true;
}

bool PSPmtMap::check(const int &location, bool &isDynode, bool &isRight, unsigned short &tqdcIndex) const {
	bool retval = false;
	
	if(location == channels[0][0] || location == channels[1][0]){ // A dynode
		isRight = (location % 2 != 0);
		isDynode = true;
		retval = true;
		tqdcIndex = 0;
	}
	else{ // Possibly an anode
		for(size_t i = 0; i < 2; i++){
			for(size_t j = 1; j < 5; j++){
				if(location == channels[i][j]){
					isRight = (i == 1);
					isDynode = false;
					retval = true;
					tqdcIndex = j;
				}
			}
		}	
	}

	return retval;
}

bool PSPmtMap::checkLocations() const {
	if(!isDoubleSided){ // Single sided
		for(size_t j = 0; j < 5; j++){
			if(channels[0][j] < 0) return false;
		}
	}
	else{ // Double sided
		for(size_t i = 0; i < 2; i++){
			for(size_t j = 0; j < 5; j++){
				if(channels[i][j] < 0) return false;
			}
		}
	}
	return true;
}

void PSPmtMap::setDynodes(const int &dynode){
	channels[0][0] = dynode;
}

void PSPmtMap::setDynodes(const int &dynodeL, const int &dynodeR){
	channels[0][0] = dynodeL;
	channels[1][0] = dynodeR;
}

void PSPmtMap::setAnodes(const int &se, const int &ne, const int &nw, const int &sw){
	channels[0][1] = se;
	channels[0][2] = ne;
	channels[0][3] = nw;
	channels[0][4] = sw;
}

void PSPmtMap::setAnodes(const int &seL, const int &neL, const int &nwL, const int &swL, const int &seR, const int &neR, const int &nwR, const int &swR){
	setAnodes(seL, neL, nwL, swL);
	channels[1][1] = seR;
	channels[1][2] = neR;
	channels[1][3] = nwR;
	channels[1][4] = swR;
	isDoubleSided = true;
}

void PSPmtMap::setLocationByTag(const int &location, const std::string &subtype, const std::string &tag){
	unsigned short index = getIndexFromTag(tag);
	if(!subtype.empty()){ // Double sided
		if(subtype == "left")
			channels[0][index] = location;
		else if(subtype == "right")
			channels[1][index] = location;
		else{ // Unknown subtype
			//errStr << "PSPmtMap: ERROR! Unknown subtype (" << subtype << ")!\n";
			std::cout << "PSPmtMap: ERROR! Unknown subtype (" << subtype << ")!\n";
		}
	}
	else{ // Single sided
		channels[0][index] = location;
	}
}

bool PSPmtMap::setNextLocation(const int &location){
	if(index2 >= 5){
		index1++;
		index2 = 0;
	}
	if(index1 >= 2) return false;
	channels[index1][index2++] = location;
	return true;
}

void PSPmtMap::print() const {
	if(!isDoubleSided)
		std::cout << "d=" << channels[0][0] << ", a={" << channels[0][1] << ", " << channels[0][2] << ", " << channels[0][3] << ", " << channels[0][4] << "}\n";
	else{
		std::cout << "LEFT: d=" << channels[0][0] << ", a={" << channels[0][1] << ", " << channels[0][2] << ", " << channels[0][3] << ", " << channels[0][4] << "}\n";
		std::cout << "RGHT: d=" << channels[1][0] << ", a={" << channels[1][1] << ", " << channels[1][2] << ", " << channels[1][3] << ", " << channels[1][4] << "}\n";
	}
}

///////////////////////////////////////////////////////////////////////////////
// class PSPmtEvent
///////////////////////////////////////////////////////////////////////////////

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

	// Get all single sided PSPMT detectors from the map
	std::vector<int> singleSidedDynodes;
	std::vector<int> singleSidedAnodes;
	mapfile->GetListOfLocations(singleSidedDynodes, "pspmt", "", true);
	mapfile->GetListOfLocations(singleSidedAnodes, "pspmt", "", true, "V1 V2 V3 V4 SE NE NW SW", true);
	
	std::vector<PSPmtMap> singleDetMap;
	for(std::vector<int>::iterator iter = singleSidedDynodes.begin(); iter != singleSidedDynodes.end(); iter++){
		singleDetMap.push_back(PSPmtMap((*iter)));
	}
	int mod, chan;
	std::string tag, subtype;
	std::vector<PSPmtMap>::iterator detector = singleDetMap.begin();
	for(std::vector<int>::iterator iter = singleSidedAnodes.begin(); iter != singleSidedAnodes.end(); iter++){
		if(detector == singleDetMap.end()) break;
		mapfile->GetModChan((*iter), mod, chan);
		tag = mapfile->GetTag(mod, chan);
		subtype = mapfile->GetSubtype(mod, chan);
		detector->setLocationByTag((*iter), subtype, tag);
		if(detector->checkLocations())
			detector++;
	}
	
	// Get all double sided PSPMT detectors from the map
	std::vector<int> doubleSidedDynodes;
	std::vector<int> doubleSidedAnodes;
	mapfile->GetListOfLocations(doubleSidedDynodes, "pspmt", "left right", true);	
	mapfile->GetListOfLocations(doubleSidedAnodes, "pspmt", "left right", true, "V1 V2 V3 V4 SE NE NW SW", true);

	std::vector<PSPmtMap> doubleDetMap;
	for(std::vector<int>::iterator iter = doubleSidedDynodes.begin(); iter != doubleSidedDynodes.end(); iter++){
		if((*iter) % 2 != 0) continue; // Ignore odd dynodes
		doubleDetMap.push_back(PSPmtMap((*iter), (*iter)+1));
	}
	detector = doubleDetMap.begin();
	for(std::vector<int>::iterator iter = doubleSidedAnodes.begin(); iter != doubleSidedAnodes.end(); iter++){
		if(detector == doubleDetMap.end()) break;
		mapfile->GetModChan((*iter), mod, chan);
		tag = mapfile->GetTag(mod, chan);
		subtype = mapfile->GetSubtype(mod, chan);
		detector->setLocationByTag((*iter), subtype, tag);
		if(detector->checkLocations())
			detector++;
	}

	// Build the list of all PSPMT detectors
	for(detector = singleDetMap.begin(); detector != singleDetMap.end(); detector++){
		detMap.push_back((*detector));
	}	
	for(detector = doubleDetMap.begin(); detector != doubleDetMap.end(); detector++){
		detMap.push_back((*detector));
	}
	
	// Sort detectors by channel location
	sort(detMap.begin(), detMap.end(), &compareTime);

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
