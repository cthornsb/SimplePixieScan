
#include <iostream>
#include <algorithm>

#include "PSPmtMap.hpp"
#include "MapFile.hpp"

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

bool PSPmtMap::check(const int &location) const {
	for(size_t i = 0; i < 2; i++){
		for(size_t j = 0; j < 5; j++){
			if(location == channels[i][j])
				return true;
		}
	}
	return false;
}

bool PSPmtMap::check(const int &location, bool &isDynode, bool &isRight, unsigned short &tqdcIndex) const {
	bool retval = false;
	
	if(location == channels[0][0]){ // A dynode
		isRight = false;
		isDynode = true;
		retval = true;
		tqdcIndex = 0;
	}
	else if(location == channels[1][0]){
		isRight = true;
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
					tqdcIndex = j-1;
					break;
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

bool PSPmtMap::readMapFile(MapFile *map, std::vector<PSPmtMap> &detMap){
	detMap.clear();

	MapEntryValidator dynodeValid;
	MapEntryValidator anodeValid;

	// Get all single sided PSPMT detectors from the map
	std::vector<int> singleSidedDynodes;
	std::vector<int> singleSidedAnodes;

	// I'll clean these up later CRT	
	dynodeValid.SetValidationMode(MapEntryValidator::WITH_TYPE | MapEntryValidator::NO_SUBTYPE | MapEntryValidator::WITHOUT_TAG);
	dynodeValid.SetValid("pspmt"); 
	dynodeValid.SetInvalid("", "", "V1 V2 V3 V4 SE NE NW SW");
	map->GetListOfLocations(singleSidedDynodes, dynodeValid);

	// I'll clean these up later CRT	
	anodeValid.SetValidationMode(MapEntryValidator::WITH_TYPE | MapEntryValidator::NO_SUBTYPE | MapEntryValidator::WITH_TAG | MapEntryValidator::FORCE_TAG);
	anodeValid.SetValid("pspmt", "", "V1 V2 V3 V4 SE NE NW SW"); 
	map->GetListOfLocations(singleSidedAnodes, anodeValid);

	std::vector<PSPmtMap> singleDetMap;
	for(std::vector<int>::iterator iter = singleSidedDynodes.begin(); iter != singleSidedDynodes.end(); iter++){
		singleDetMap.push_back(PSPmtMap((*iter)));
	}
	int mod, chan;
	std::string tag, subtype;
	std::vector<PSPmtMap>::iterator detector = singleDetMap.begin();
	for(std::vector<int>::iterator iter = singleSidedAnodes.begin(); iter != singleSidedAnodes.end(); iter++){
		if(detector == singleDetMap.end()) break;
		map->GetModChan((*iter), mod, chan);
		tag = map->GetTag(mod, chan);
		subtype = map->GetSubtype(mod, chan);
		detector->setLocationByTag((*iter), subtype, tag);
		if(detector->checkLocations())
			detector++;
	}

	// Get all double sided PSPMT detectors from the map
	std::vector<int> doubleSidedDynodes;
	std::vector<int> doubleSidedAnodes;

	// I'll clean these up later CRT
	dynodeValid.SetValidationMode(MapEntryValidator::WITH_TYPE | MapEntryValidator::WITH_SUBTYPE | MapEntryValidator::WITHOUT_TAG);
	dynodeValid.SetValid("pspmt", "left right"); 
	dynodeValid.SetInvalid("", "left right", "V1 V2 V3 V4 SE NE NW SW");
	map->GetListOfLocations(doubleSidedDynodes, dynodeValid);

	// I'll clean these up later CRT
	anodeValid.SetValidationMode(MapEntryValidator::WITH_TYPE | MapEntryValidator::WITH_SUBTYPE | MapEntryValidator::WITH_TAG | MapEntryValidator::FORCE_TAG);
	anodeValid.SetValid("pspmt", "left right", "V1 V2 V3 V4 SE NE NW SW"); 
	map->GetListOfLocations(doubleSidedAnodes, anodeValid);

	std::vector<PSPmtMap> doubleDetMap;
	for(std::vector<int>::iterator iter = doubleSidedDynodes.begin(); iter != doubleSidedDynodes.end(); iter++){
		if((*iter) % 2 != 0) continue; // Ignore odd dynodes
		doubleDetMap.push_back(PSPmtMap((*iter), (*iter)+1));
	}
	detector = doubleDetMap.begin();
	for(std::vector<int>::iterator iter = doubleSidedAnodes.begin(); iter != doubleSidedAnodes.end(); iter++){
		if(detector == doubleDetMap.end()) break;
		map->GetModChan((*iter), mod, chan);
		tag = map->GetTag(mod, chan);
		subtype = map->GetSubtype(mod, chan);
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
	
	return !detMap.empty();
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
	dynTime = 0;
	dynLTQDC = 0;
	dynSTQDC = 0;
	dynodeSet = false;
	for(size_t i = 0; i < 4; i++) 
		channels[i] = false;
}

void PSPmtEvent::addDynode(const double &time, const double &L, const double &S){
	dynTime = time;
	dynLTQDC = L;
	dynSTQDC = S;
	dynodeSet = true;
}

void PSPmtEvent::addAnode(const float &anode, const size_t &index){
	if(anode > 0){
		anodes[index] = anode;
		channels[index] = true;
	}
}

bool PSPmtEvent::allValuesSet(){
	if(!dynodeSet) return false;
	for(size_t i = 0; i < 4; i++)
		if(!channels[i]) return false;
	xpos = ((anodes[0]+anodes[1])-(anodes[2]+anodes[3]))/(anodes[0]+anodes[1]+anodes[2]+anodes[3]);
	ypos = ((anodes[1]+anodes[2])-(anodes[0]+anodes[3]))/(anodes[0]+anodes[1]+anodes[2]+anodes[3]);
	return true;
}
