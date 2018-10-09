#include <iostream>
#include <time.h>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "CTerminal.h"

#include "simpleTool.hpp"
#include "Structures.hpp"
#include "pspmt.hpp"

template <typename T>
void writeTNamed(const char *label_, const T &val_, const int &precision_=-1){
	std::stringstream stream; 
	if(precision_ < 0) 
		stream << val_;
	else{ // Set the precision of the output stream.
		stream.precision(precision_);
		stream << std::fixed << val_;
	}
	TNamed named(label_, stream.str().c_str());
	named.Write();
}

fullBarEvent* buildBarEvent(pspmtMapEntry *entryL_, pspmtMapEntry *entryR_){
	if(!entryL_->check() || !entryR_->check()) return NULL;
	std::deque<simpleEvent*>* evtArrayL = entryL_->getEvents();
	std::deque<simpleEvent*>* evtArrayR = entryR_->getEvents();
        fullBarEvent *evt = new fullBarEvent(evtArrayL[0].front(), evtArrayL[1].front(), evtArrayL[2].front(), evtArrayL[3].front(), evtArrayL[4].front(),
	                                     evtArrayR[0].front(), evtArrayR[1].front(), evtArrayR[2].front(), evtArrayR[3].front(), evtArrayR[4].front());
	for(size_t i = 0; i < 5; i++){ // What to do with higher multiplicity? CRT
		evtArrayL[i].pop_front();
		evtArrayR[i].pop_front();
	}
	return evt;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleEvent
///////////////////////////////////////////////////////////////////////////////

simpleEvent::simpleEvent(PSPmtStructure *ptr, const size_t &index){
	tdiff = ptr->tdiff.at(index);
	ltqdc = ptr->ltqdc.at(index);
	stqdc = ptr->stqdc.at(index);
	location = ptr->loc.at(index);

	unsigned short chanIdentifier = ptr->chan.at(index);
	
	// Decode the channel information.
	isBarDet   = ((chanIdentifier & 0x0010) != 0);
	isRightEnd = ((chanIdentifier & 0x0020) != 0);	
	tqdcIndex  =  (chanIdentifier & 0x00C0) >> 6;
	//= (chanIdentifier & 0xFF00) >> 8; // Remaining 8 bits. Currently un-used.
}

///////////////////////////////////////////////////////////////////////////////
// class fullEvent
///////////////////////////////////////////////////////////////////////////////

fullEvent::fullEvent(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW){
	compute(dynode, anode_SE, anode_NE, anode_NW, anode_SW);
}

void fullEvent::compute(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW){
	tdiff = dynode->tdiff;
	
	ltqdc[0] = anode_SE->ltqdc;
	ltqdc[1] = anode_NE->ltqdc;
	ltqdc[2] = anode_NW->ltqdc;
	ltqdc[3] = anode_SW->ltqdc;
	
	ltqdcSum = ltqdc[0]+ltqdc[1]+ltqdc[2]+ltqdc[3];
	stqdcSum = stqdc[0]+stqdc[1]+stqdc[2]+stqdc[3];
	
	xpos = ((ltqdc[0]+ltqdc[1])-(ltqdc[2]+ltqdc[3]))/ltqdcSum;
	ypos = ((ltqdc[1]+ltqdc[2])-(ltqdc[0]+ltqdc[3]))/ltqdcSum;
	
	loc = dynode->location;
}

///////////////////////////////////////////////////////////////////////////////
// class fullBarEvent
///////////////////////////////////////////////////////////////////////////////
	
fullBarEvent::fullBarEvent(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
                           simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R){
	compute(dynode_L, anode_SE_L, anode_NE_L, anode_NW_L, anode_SW_L, dynode_R, anode_SE_R, anode_NE_R, anode_NW_R, anode_SW_R);
}

void fullBarEvent::compute(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
                           simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R){
	tdiff_L = dynode_L->tdiff;
	tdiff_R = dynode_R->tdiff;
	
	ltqdc_L[0] = anode_SE_L->ltqdc;
	ltqdc_L[1] = anode_NE_L->ltqdc;
	ltqdc_L[2] = anode_NW_L->ltqdc;
	ltqdc_L[3] = anode_SW_L->ltqdc;

	ltqdc_R[0] = anode_SE_R->ltqdc;
	ltqdc_R[1] = anode_NE_R->ltqdc;
	ltqdc_R[2] = anode_NW_R->ltqdc;
	ltqdc_R[3] = anode_SW_R->ltqdc;

	stqdc_L[0] = anode_SE_L->stqdc;
	stqdc_L[1] = anode_NE_L->stqdc;
	stqdc_L[2] = anode_NW_L->stqdc;
	stqdc_L[3] = anode_SW_L->stqdc;

	stqdc_R[0] = anode_SE_R->stqdc;
	stqdc_R[1] = anode_NE_R->stqdc;
	stqdc_R[2] = anode_NW_R->stqdc;
	stqdc_R[3] = anode_SW_R->stqdc;
	
	ltqdcSum_L = ltqdc_L[0]+ltqdc_L[1]+ltqdc_L[2]+ltqdc_L[3];
	ltqdcSum_R = ltqdc_R[0]+ltqdc_R[1]+ltqdc_R[2]+ltqdc_R[3];

	stqdcSum_L = stqdc_L[0]+stqdc_L[1]+stqdc_L[2]+stqdc_L[3];
	stqdcSum_R = stqdc_R[0]+stqdc_R[1]+stqdc_R[2]+stqdc_R[3];

	xpos_L = ((ltqdc_L[0]+ltqdc_L[1])-(ltqdc_L[2]+ltqdc_L[3]))/ltqdcSum_L;
	ypos_L = ((ltqdc_L[1]+ltqdc_L[2])-(ltqdc_L[0]+ltqdc_L[3]))/ltqdcSum_L;

	xpos_R = -((ltqdc_R[0]+ltqdc_R[1])-(ltqdc_R[2]+ltqdc_R[3]))/ltqdcSum_R; // Sign is flipped to preserve x-axis of left side.
	ypos_R = ((ltqdc_R[1]+ltqdc_R[2])-(ltqdc_R[0]+ltqdc_R[3]))/ltqdcSum_R;
	
	loc = dynode_L->location;
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtMapFileEntry
///////////////////////////////////////////////////////////////////////////////

unsigned int pspmtMapFileEntry::ReadPars(const std::vector<std::string> &pars_){ 
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0)
			id = GetPixieID(*iter);
		else if(index == 1)
			an1 = GetPixieID(*iter);
		else if(index == 2)
			an2 = GetPixieID(*iter);
		else if(index == 3)
			an3 = GetPixieID(*iter);
		else if(index == 4)
			an4 = GetPixieID(*iter);
		index++;
	}
	return id;
}

std::string pspmtMapFileEntry::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", an1=" << an1 << ", an2=" << an2 << ", an3=" << an3 << ", an4=" << an4;
	else output << id << "\t" << an1 << "\t" << an2 << "\t" << an3 << "\t" << an4 << "\n";
	return output.str();
}

void pspmtMapFileEntry::GetIDs(unsigned short *arr){
	arr[0] = id;
	arr[1] = an1;
	arr[2] = an2;
	arr[3] = an3;
	arr[4] = an4;
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtMapEntry
///////////////////////////////////////////////////////////////////////////////

pspmtMapEntry::pspmtMapEntry(){
	for(size_t i = 0; i < 5; i++){
		mult[i] = 0;
		ids[i] = 0;
	}
}

pspmtMapEntry::pspmtMapEntry(unsigned short *ptr_){
	for(size_t i = 0; i < 5; i++){
		mult[i] = 0;
		ids[i] = ptr_[i];
	}
}

void pspmtMapEntry::clear(){
	for(size_t i = 0; i < 5; i++){
		events[i].clear();
		mult[i] = 0;
	}
}

bool pspmtMapEntry::add(simpleEvent *evt){
	for(size_t i = 0; i < 5; i++){
		if(evt->location == ids[i]){
			events[i].push_back(evt);
			mult[i]++;
			return true;
		}
	}
	return false;
}

bool pspmtMapEntry::check(){
	for(size_t i = 0; i < 5; i++){
		if(mult[i] == 0) return false;
	}
	return true;
}

fullEvent* pspmtMapEntry::buildEvent(){
	if(!this->check()) return NULL;
	fullEvent *evt = new fullEvent(events[0].front(), events[1].front(), events[2].front(), events[3].front(), events[4].front());
	for(size_t i = 0; i < 5; i++){
		events[i].pop_front(); // What to do with higher multiplicity? CRT
	}
	return evt;
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtMap
///////////////////////////////////////////////////////////////////////////////

void pspmtMap::addEntry(unsigned short *ptr_){
	entries.push_back(pspmtMapEntry(ptr_));
}

bool pspmtMap::addEvent(simpleEvent *evt_){
	for(std::vector<pspmtMapEntry>::iterator iter = entries.begin(); iter != entries.end(); iter++){
		if(iter->add(evt_)) return true;
	}
	return false;
}

void pspmtMap::clear(){
	for(std::vector<pspmtMapEntry>::iterator iter = entries.begin(); iter != entries.end(); iter++){
		iter->clear();
	}
}

void pspmtMap::buildEventList(std::vector<fullEvent*>& vec_){
	for(std::vector<pspmtMapEntry>::iterator iter = entries.begin(); iter != entries.end(); iter++){
		if(iter->check()) vec_.push_back(iter->buildEvent());
		iter->clear();
	}
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtBarMap
///////////////////////////////////////////////////////////////////////////////

void pspmtBarMap::addEntry(unsigned short *ptrL_, unsigned short *ptrR_){
	mapL.addEntry(ptrL_);
	mapR.addEntry(ptrR_);
}

void pspmtBarMap::addLeftEntry(unsigned short *ptr_){
	mapL.addEntry(ptr_);
}

void pspmtBarMap::addRightEntry(unsigned short *ptr_){
	mapR.addEntry(ptr_);
}

bool pspmtBarMap::addEvent(simpleEvent *evtL_, simpleEvent *evtR_){
	return (mapL.addEvent(evtL_) && mapR.addEvent(evtR_));
}

bool pspmtBarMap::addLeftEvent(simpleEvent *evt_){
	return mapL.addEvent(evt_);
}

bool pspmtBarMap::addRightEvent(simpleEvent *evt_){
	return mapR.addEvent(evt_);
}

void pspmtBarMap::clear(){
	mapL.clear();
	mapR.clear();
}

void pspmtBarMap::buildEventList(std::vector<fullBarEvent*>& vec_){
	std::vector<pspmtMapEntry>::iterator iterL = mapL.getBegin();
	std::vector<pspmtMapEntry>::iterator iterR = mapR.getBegin();

	for(; iterL != mapL.getEnd() && iterR != mapR.getEnd(); iterL++, iterR++){
		fullBarEvent *evt = buildBarEvent(&(*iterL), &(*iterR));
		if(evt) vec_.push_back(evt);
		iterL->clear();
		iterR->clear();
	}
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtHandler
///////////////////////////////////////////////////////////////////////////////

class pspmtHandler : public simpleTool {
  private:
	std::string setupDir;

	unsigned short index;

	CalibFile calib;

	pspmtMap map;
	pspmtBarMap barmap;
	
	PSPmtStructure *ptr;

	bool singleEndedMode;
	bool noTimeMode;
	bool noEnergyMode;
	bool noPositionMode;

	std::string countsString;
	unsigned long long totalCounts;
	double totalDataTime;

	double x, y, z, r, theta;
	double ctof, tqdc, stqdc, ctqdc, energy;
	double xdetL, xdetR, ydetL, ydetR;
	unsigned short location;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;
	float stqdc_L, stqdc_R;	

	float allTQDC_L[4];
	float allTQDC_R[4];

	void process();

	void handleEvents();

	void setVariables(fullEvent* evt_);

	void setVariables(fullBarEvent* evt_);

  public:
	pspmtHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), map(), barmap(), ptr(NULL), singleEndedMode(false), noTimeMode(false), noEnergyMode(false), 
	                 noPositionMode(false), totalCounts(0), totalDataTime(0) { }

	~pspmtHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void pspmtHandler::process(){
	double ctdiff, cylTheta, dW, alpha, rprime;

	// Check for invalid TQDC.
	if(tqdc_L <= 0 || (!singleEndedMode && tqdc_R <= 0)) return;

	BarCal *bar = NULL;
	if(!singleEndedMode && !(bar = calib.GetBarCal(location))) return;

	TimeCal *time = NULL;
	if(!noTimeMode && !(time = calib.GetTimeCal(location))) return;

	PositionCal *pos = NULL;
	if(!noPositionMode){
		if(!(pos = calib.GetPositionCal(location))) return;

		// Angle of center of bar in cylindrical coordinates.
		cylTheta = pos->theta;

		// Take the width of the bar into consideration.
		if(!singleEndedMode){
			dW = frand(-bar->width/200, bar->width/200);
			rprime = std::sqrt(pos->r0*pos->r0 + dW*dW);
			alpha = std::atan2(dW, pos->r0);
			addAngles(cylTheta, alpha);
		}
		else rprime = pos->r0;

		// Calculate the x and z position of the event.
		x = rprime*std::sin(cylTheta); // m
		z = rprime*std::cos(cylTheta); // m
	}
	else{
		x = 0;
		z = 0;
	}

	// Compute the corrected time difference
	if(!singleEndedMode){
		ctdiff = tdiff_R - tdiff_L - bar->t0;
		y = bar->cbar*ctdiff/200; // m

		// Check for invalid radius.
		if(y < -bar->length/30.0 || y > bar->length/30.0) return;
	}
	else y = 0; // m

	// Calculate the spherical polar angle.
	if(!noPositionMode){
		r = std::sqrt(x*x + y*y + z*z);
		theta = std::acos(z/r)*180/pi;
		//phi = std::acos(y/std::sqrt(x*x + y*y); }
		//if(x < 0) phi = 2*pi - phi; 
	}
	else{
		r = 0;
		theta = 0;
	}

	if(!singleEndedMode){
		// Calculate the corrected TOF.
		if(!noPositionMode){
			if(!noTimeMode)
				ctof = (pos->r0/r)*((tdiff_R + tdiff_L)/2 - time->t0) + 100*pos->r0/cvac;
			else
				ctof = (pos->r0/r)*((tdiff_R + tdiff_L)/2) + 100*pos->r0/cvac;
		}
		else if(!noTimeMode)
			ctof = (tdiff_R + tdiff_L)/2 - time->t0;
		else
			ctof = (tdiff_R + tdiff_L)/2;

		// Calculate the TQDC.
		tqdc = std::sqrt(tqdc_R*tqdc_L);
	}
	else{
		if(!noPositionMode){
			if(!noTimeMode)
				ctof = tdiff_L - time->t0 + 100*pos->r0/cvac;
			else
				ctof = tdiff_L + 100*pos->r0/cvac;
		}
		else if(!noTimeMode)
			ctof = tdiff_L - time->t0;
		else
			ctof = tdiff_L;
		tqdc = tqdc_L;
	}

	// Calculate the short integral for PSD.
	if(!singleEndedMode)
		stqdc = std::sqrt(stqdc_R*stqdc_L);
	else
		stqdc = stqdc_L;

	if(!noEnergyMode){// Calibrate the TQDC.
		if(!singleEndedMode){
			EnergyCal *ecalLeft = calib.GetEnergyCal(location);
			EnergyCal *ecalRight = calib.GetEnergyCal(location+1);
			if(ecalLeft && !ecalLeft->defaultVals){
				if(!ecalRight || ecalRight->defaultVals){ // Use pairwise calibration.
					ctqdc = ecalLeft->GetCalEnergy(std::sqrt(tqdc_R*tqdc_L));
				}
				else{ // Use individual channel calibration.
					tqdc_L = ecalLeft->GetCalEnergy(tqdc_L);
					tqdc_R = ecalRight->GetCalEnergy(tqdc_R);
					ctqdc = std::sqrt(tqdc_R*tqdc_L);
				}
			}
		}
		else{
			EnergyCal *ecal = calib.GetEnergyCal(location);
			if(ecal) ctqdc = ecal->GetCalEnergy(tqdc);
		}
	}

            // Calculate the neutron energy.
	if(!noPositionMode) energy = tof2energy(ctof, pos->r0);

	// Fill the tree with the event.
	outtree->Fill();
}

void pspmtHandler::handleEvents(){
	std::vector<simpleEvent> events;
	
	// First, we copy all the independent signals into a vector.
	for(unsigned short index = 0; index < ptr->mult; index++){
		events.push_back(simpleEvent(ptr, index));
	}
	
	// Next, we use pspmtMap to sort events into 5-channel PSPMT detectors.
	for(std::vector<simpleEvent>::iterator iter = events.begin(); iter != events.end(); iter++){
		if(!singleEndedMode){
			if(!iter->isRightEnd) // Left end PSPMT
				barmap.addLeftEvent(&(*iter));
			else // Right end PSPMT
				barmap.addRightEvent(&(*iter));
		}
		else
			map.addEvent(&(*iter));
	}
	
	// Now construct fullEvents using the map.
	if(!singleEndedMode){ // Double-ended
		std::vector<fullBarEvent*> fullEvents;
		barmap.buildEventList(fullEvents);	

		// How many PSPMT events did we get? Temporary CRT
		//std::cout << " built " << fullEvents.size() << " full bar events from " << events.size() << " simple events\n";
		
		// Clean up	
		for(std::vector<fullBarEvent*>::iterator iter = fullEvents.begin(); iter != fullEvents.end(); iter++){
			setVariables((*iter));
			process();
			delete (*iter);
		}
		fullEvents.clear();
		events.clear();
	}
	else{ // Single-ended
		std::vector<fullEvent*> fullEvents;
		map.buildEventList(fullEvents);		

		// How many PSPMT events did we get? Temporary CRT
		//std::cout << " built " << fullEvents.size() << " full events from " << events.size() << " simple events\n";
	
		// Clean up	
		for(std::vector<fullEvent*>::iterator iter = fullEvents.begin(); iter != fullEvents.end(); iter++){
			setVariables((*iter));
			process();
			delete (*iter);
		}
		fullEvents.clear();
		events.clear();
	}
}

void pspmtHandler::setVariables(fullEvent *evt_){
	tdiff_L = evt_->tdiff;
	tqdc_L = evt_->ltqdcSum;
	stqdc_L = evt_->stqdcSum;
	location = evt_->loc;

	xdetL = evt_->xpos;
	ydetL = evt_->ypos;

	if(debug){
		for(size_t i = 0; i < 4; i++) allTQDC_L[i] = evt_->ltqdc[i];
	}
}

void pspmtHandler::setVariables(fullBarEvent *evt_){
	tdiff_L = evt_->tdiff_L;
	tdiff_R = evt_->tdiff_R;
	tqdc_L = evt_->ltqdcSum_L;
	tqdc_R = evt_->ltqdcSum_R;
	stqdc_L = evt_->stqdcSum_L;
	stqdc_R = evt_->stqdcSum_R;
	location = evt_->loc;

	xdetL = evt_->xpos_L;
	ydetL = evt_->ypos_L;
	xdetR = evt_->xpos_R;
	ydetR = evt_->ypos_R;

	if(debug){
		for(size_t i = 0; i < 4; i++){
			allTQDC_L[i] = evt_->ltqdc_L[i];
			allTQDC_R[i] = evt_->ltqdc_R[i];
		}
	}
}

pspmtHandler::~pspmtHandler(){
}

void pspmtHandler::addOptions(){
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read bar speed-of-light from an input cal file."), userOpts, optstr);
	addOption(optionExt("single", no_argument, NULL, 0x0, "", "Single-ended PSPMT detector mode."), userOpts, optstr);
	addOption(optionExt("debug", no_argument, NULL, 0x0, "", "Enable debug output."), userOpts, optstr);
	addOption(optionExt("no-energy", no_argument, NULL, 0x0, "", "Do not use energy calibration."), userOpts, optstr);
	addOption(optionExt("no-time", no_argument, NULL, 0x0, "", "Do not use time calibration."), userOpts, optstr);
	addOption(optionExt("no-position", no_argument, NULL, 0x0, "", "Do not use position calibration."), userOpts, optstr);
}

bool pspmtHandler::processArgs(){
	if(userOpts.at(0).active){
		setupDir = userOpts.at(0).argument;
		if(setupDir[setupDir.size()-1] != '/') setupDir += '/';
	}
	if(userOpts.at(1).active){ // Single-ended PSPMT
		singleEndedMode = true;
	}
	if(userOpts.at(2).active){ // Debug output
		debug = true;
	}
	if(userOpts.at(3).active){
		noEnergyMode = true;
	}
	if(userOpts.at(4).active){
		noTimeMode = true;
	}
	if(userOpts.at(5).active){
		noPositionMode = true;
	}

	return true;
}

int pspmtHandler::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	// Seed the random number generator.
	srand(time(NULL));

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	int numLinesRead = 0;
	std::vector<pspmtMapFileEntry> pspmtMapFileEntries;
	if(!LoadCalibFile((setupDir+"pspmt.dat").c_str(), pspmtMapFileEntries)) return 2;
	for(size_t i = 0; i < pspmtMapFileEntries.size(); i++){
		if(pspmtMapFileEntries.at(i).defaultVals) continue;
		unsigned short array[5];
		pspmtMapFileEntries.at(i).GetIDs(array);
		if(!singleEndedMode){
			if(numLinesRead % 2 == 0) // Even (left)
				barmap.addLeftEntry(array);
			else // Odd (right)
				barmap.addRightEntry(array);
		}
		else
			map.addEntry(array);
		numLinesRead++;
	}

	std::cout << " Loaded " << numLinesRead << " PSPMT detectors from pspmt map file.\n";	

	if(!noPositionMode && !calib.LoadPositionCal((setupDir+"position.cal").c_str())) return 3;
	if(!noTimeMode && !calib.LoadTimeCal((setupDir+"time.cal").c_str())) return 4;
	if(!noEnergyMode && !calib.LoadEnergyCal((setupDir+"energy.cal").c_str())) return 5;
	if(!singleEndedMode && !calib.LoadBarCal((setupDir+"bars.cal").c_str())) return 6;

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 7;
	}
	
	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 8;
	}

	outtree = new TTree("data", "Processed PSPMT data");

	outtree->Branch("ctof", &ctof);
	if(!noPositionMode)
		outtree->Branch("energy", &energy);
	outtree->Branch("tqdc", &tqdc);
	outtree->Branch("stqdc", &stqdc);
	if(!noEnergyMode)
		outtree->Branch("ctqdc", &ctqdc);
	if(!noPositionMode){
		outtree->Branch("r", &r);
		outtree->Branch("theta", &theta);
		outtree->Branch("x", &x);
		outtree->Branch("y", &y);
		outtree->Branch("z", &z);
	}
	if(!singleEndedMode){
		outtree->Branch("xdetL", &xdetL);
		outtree->Branch("ydetL", &ydetL);
		outtree->Branch("xdetR", &xdetR);
		outtree->Branch("ydetR", &ydetR);
		if(debug){
			outtree->Branch("anodeL[4]", allTQDC_L);
			outtree->Branch("anodeR[4]", allTQDC_R);
		}
	}
	else{
		outtree->Branch("xdet", &xdetL);
		outtree->Branch("ydet", &ydetL);
		if(debug) outtree->Branch("anode[4]", allTQDC_L);
	}
	outtree->Branch("loc", &location);

	int file_counter = 1;
	while(openInputFile()){
		std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

		if(!loadInputTree()){ // Load the input tree.
			std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
			continue;
		}

		TBranch *branch = NULL;
		intree->SetBranchAddress("pspmt", &ptr, &branch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"pspmt\" from input TTree.\n";
			return 9;
		}

		// Get the data time from the input file.
		TNamed *named;
		infile->GetObject("head/file01/Data time", named);
		if(named){
			double fileDataTime = strtod(named->GetTitle(), NULL);
			if(fileDataTime > 0)
				totalDataTime += fileDataTime;
			else
				std::cout << " Warning: Encountered negative data time (" << fileDataTime << " s)\n";
		}
		else 
			std::cout << " Warning: Failed to find pixie data time in input file!\n";

		// Handle all events.
		while(getNextEntry())
			handleEvents();
	}

	// Write output tree to file.
	outfile->cd();
	outtree->Write();

	// Write calibration information to output file.
	calib.Write(outfile);

	// Write the total number of logic counts, if enabled.
	outfile->cd();
	if(totalCounts > 0) 
		writeTNamed(countsString.c_str(), totalCounts);

	// Write the total data time.
	writeTNamed("time", totalDataTime, 1);

	std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
			
	return 0;
}

int main(int argc, char *argv[]){
	pspmtHandler obj;

	return obj.execute(argc, argv);
}
