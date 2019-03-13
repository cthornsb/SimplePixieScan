#include <iostream>
#include <time.h>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TF1.h"
#include "TGraph.h"
#include "TSpectrum.h"
#include "TH1.h"
#include "TH2.h"
#include "TLine.h"
#include "TCutG.h"

#include "CTerminal.h"

#include "simpleTool.hpp"
#include "Structures.hpp"
#include "pspmt.hpp"

const double height=0.0508; // m
const double width=0.0508; // m

const double halfHeight=height/2;
const double halfWidth=width/2;

const int NcellsX=8;
const int NcellsY=4;

const double xSpacing=width/NcellsX;
const double ySpacing=height/NcellsY;

// This is global to save a lot of headaches passing it around everywhere.
bool useFilterEnergy=false;

bool compare(const gPar &l, const gPar &r){ return (l.p1 < r.p1); }

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
	if(!entryL_->check() && !entryR_->check()) return NULL;
	if(!entryL_->checkDynode() || !entryR_->checkDynode()) return NULL;
	std::deque<simpleEvent*>* evtArrayL = entryL_->getEvents();
	std::deque<simpleEvent*>* evtArrayR = entryR_->getEvents();
	/*for(size_t i = 0; i < 5; i++){
		std::cout << i << "\t" << evtArrayL[i].size() << "\t" << evtArrayR[i].size() << std::endl;
	}*/
	fullBarEvent *evt = new fullBarEvent(evtArrayL, evtArrayR);
        //fullBarEvent *evt = new fullBarEvent(evtArrayL[0].front(), evtArrayL[1].front(), evtArrayL[2].front(), evtArrayL[3].front(), evtArrayL[4].front(),
	//                                     evtArrayR[0].front(), evtArrayR[1].front(), evtArrayR[2].front(), evtArrayR[3].front(), evtArrayR[4].front());
	/*for(size_t i = 0; i < 5; i++){ // What to do with higher multiplicity? CRT
		evtArrayL[i].pop_front();
		evtArrayR[i].pop_front();
	}*/
	return evt;
}

std::string getFuncStr(int nPeak){
	std::stringstream retval;
	retval << "gaus";
	for(int i = 1; i < nPeak; i++)
		retval << "+gaus(" << 3*i << ")";
	retval << "+pol0(" << 3*nPeak << ")";
	return retval.str();
}

std::string getDrawStr(TF1 *f, int nPeak, const std::string &var){
	std::stringstream retval;
	retval << f->GetParameter(0);
	for(int i = 1; i < nPeak; i++){
		if(f->GetParameter(i) >= 0) retval << "+";
		retval << f->GetParameter(i) << "*" << var;
		if(i > 1) retval << "^" << i;
	}
	return retval.str();
}

void setInitPars(TF1 *f, TSpectrum *spec, int nPeak, double sigma=0.05){
	for(int i = 0; i < nPeak; i++){
		f->SetParameter(3*i, spec->GetPositionY()[i]);
		f->SetParameter(3*i+1, spec->GetPositionX()[i]);
		f->SetParameter(3*i+2, sigma);
		//f->SetParLimits(3*i+2, 0.025, 0.075);
	}
	f->SetParameter(3*nPeak, 20);
}

short getXcell(const double &xpos){
	return (short)((xpos+halfWidth)/xSpacing);
}

short getYcell(const double &ypos){
	return (short)((ypos+halfHeight)/ySpacing);
}

///////////////////////////////////////////////////////////////////////////////
// class pspmtPosCal
///////////////////////////////////////////////////////////////////////////////

double pspmtPosCal::calX(const double &x0_, double *par_){
	return (par_[0] + par_[1]*x0_ + par_[2]*x0_*x0_ + par_[3]*x0_*x0_*x0_);
}

double pspmtPosCal::calY(const double &y0_){
	return (yp[0] + yp[1]*y0_ + yp[2]*y0_*y0_ + yp[3]*y0_*y0_*y0_);
}

void pspmtPosCal::calibrate(const double &x0_, const double &y0_, double &x1, double &y1){
	y1 = this->calY(y0_);
	//if(y0_ <= ymin[0])
		x1 = this->calX(x0_, xp[0]);
	/*else if(y0_ <= ymin[1])
		x1 = this->calX(x0_, xp[1]);
	else if(y0_ <= ymin[2])
		x1 = this->calX(x0_, xp[2]);
	else
		x1 = this->calX(x0_, xp[3]);*/
}

std::string pspmtPosCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy){
		output << " id=" << id << ", yMin0=" << ymin[0] << ", yMin1=" << ymin[1] << ", yMin2=" << ymin[2] << std::endl;
		for(int i = 0; i < 4; i++){
			output << "  x" << i << "=(" << xp[i][0] << ", " << xp[i][1] << ", " << xp[i][2] << ", " << xp[i][3] << ")\n";
		}
		output << "  y=(" << yp[0] << ", " << yp[1] << ", " << yp[2] << ", " << yp[3] << ")\n";
	}
	else{
		output << id << "\t" << xp[0] << "\t" << xp[1] << "\t" << xp[2] << "\t" << xp[3];
		output << "\t" << yp[0] << "\t" << yp[1] << "\t" << yp[2] << "\t" << yp[3];
	}
	return output.str();
}

unsigned int pspmtPosCal::ReadPars(const std::vector<std::string> &pars_){
	defaultVals = false;
	int index = 0;
	std::cout << "debug: size=" << pars_.size() << std::endl;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else if(index <= 3) // Y-axis minima
			ymin[index-1] = strtod(iter->c_str(), NULL);
		else if(index <= 7) // x parameters
			xp[0][index-4] = strtod(iter->c_str(), NULL);
		else if(index <= 11) // x parameters
			xp[1][index-8] = strtod(iter->c_str(), NULL);
		else if(index <= 15) // x parameters
			xp[2][index-12] = strtod(iter->c_str(), NULL);
		else if(index <= 19) // x parameters
			xp[3][index-16] = strtod(iter->c_str(), NULL);
		else if(index <= 23) // y parameters
			yp[index-20] = strtod(iter->c_str(), NULL);
		index++;
	}
	return id;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleEvent
///////////////////////////////////////////////////////////////////////////////

simpleEvent::simpleEvent(PSPmtStructure *ptr, const size_t &index){
	tdiff = ptr->tdiff.at(index);
	ltqdc = ptr->ltqdc.at(index);
	stqdc = ptr->stqdc.at(index);
	energy = ptr->energy.at(index);
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

	ltqdc = dynode->ltqdc;
	stqdc = dynode->stqdc;

	tqdc[0] = anode_SE->ltqdc;
	tqdc[1] = anode_NE->ltqdc;
	tqdc[2] = anode_NW->ltqdc;
	tqdc[3] = anode_SW->ltqdc;

	energy[0] = (float)anode_SE->energy;
	energy[1] = (float)anode_NE->energy;
	energy[2] = (float)anode_NW->energy;
	energy[3] = (float)anode_SW->energy;

	tqdcSum = tqdc[0]+tqdc[1]+tqdc[2]+tqdc[3];
	energySum = energy[0]+energy[1]+energy[2]+energy[3];

	if(!useFilterEnergy){
		xpos = ((tqdc[0]+tqdc[1])-(tqdc[2]+tqdc[3]))/tqdcSum;
		ypos = ((tqdc[1]+tqdc[2])-(tqdc[0]+tqdc[3]))/tqdcSum;
	}	
	else{
		xpos = ((energy[0]+energy[1])-(energy[2]+energy[3]))/energySum;
		ypos = ((energy[1]+energy[2])-(energy[0]+energy[3]))/energySum;
	}

	loc = dynode->location;
}

///////////////////////////////////////////////////////////////////////////////
// class fullBarEvent
///////////////////////////////////////////////////////////////////////////////

fullBarEvent::fullBarEvent(std::deque<simpleEvent*> *left, std::deque<simpleEvent*> *right){
	clear();

	// Read the dynode signals.
	if(!left[0].empty() && !right[0].empty())
		readDynodes(left[0].front(), right[0].front());

	// Read the left anode signals.
	if(!left[1].empty() && !left[2].empty() && !left[3].empty() && !left[4].empty())
		readLeftAnodes(left[1].front(), left[2].front(), left[3].front(), left[4].front());

	// Read the right anode signals.
	if(!right[1].empty() && !right[2].empty() && !right[3].empty() && !right[4].empty())
		readRightAnodes(right[1].front(), right[2].front(), right[3].front(), right[4].front());

	compute();
}

fullBarEvent::fullBarEvent(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
                           simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R){
	clear();
	readDynodes(dynode_L, dynode_R);
	readLeftAnodes(anode_SE_L, anode_NE_L, anode_NW_L, anode_SW_L);
	readRightAnodes(anode_SE_R, anode_NE_R, anode_NW_R, anode_SW_R);
	compute();
}

bool fullBarEvent::readDynodes(simpleEvent *dynode_L, simpleEvent *dynode_R){
	if(!dynode_L || !dynode_R) return false;

	tdiff_L = dynode_L->tdiff;
	tdiff_R = dynode_R->tdiff;

	ltqdc_L = dynode_L->ltqdc;
	ltqdc_R = dynode_R->ltqdc;
	stqdc_L = dynode_L->stqdc;
	stqdc_R = dynode_R->stqdc;

	loc = dynode_L->location;

	return true;
}

bool fullBarEvent::readLeftAnodes(simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW){
	valid_L = true;	
	valid_L &= readAnode(tqdc_L[0], energy_L[0], anode_SE);
	valid_L &= readAnode(tqdc_L[1], energy_L[1], anode_NE);
	valid_L &= readAnode(tqdc_L[2], energy_L[2], anode_NW);
	valid_L &= readAnode(tqdc_L[3], energy_L[3], anode_SW);
	return valid_L;
}

bool fullBarEvent::readRightAnodes(simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW){
	valid_R = true;	
	valid_R &= readAnode(tqdc_R[0], energy_R[0], anode_SE);
	valid_R &= readAnode(tqdc_R[1], energy_R[1], anode_NE);
	valid_R &= readAnode(tqdc_R[2], energy_R[2], anode_NW);
	valid_R &= readAnode(tqdc_R[3], energy_R[3], anode_SW);
	return valid_R;
}

bool fullBarEvent::readAnode(float &tqdc, float &energy, simpleEvent *anode){
	if(anode){
		tqdc = anode->ltqdc;
		energy = anode->energy;
		return true;
	}
	tqdc = 0;
	energy = 0;
	return false;
}

void fullBarEvent::compute(){
	tqdcSum_L = tqdc_L[0]+tqdc_L[1]+tqdc_L[2]+tqdc_L[3];
	tqdcSum_R = tqdc_R[0]+tqdc_R[1]+tqdc_R[2]+tqdc_R[3];
	energySum_L = energy_L[0]+energy_L[1]+energy_L[2]+energy_L[3];
	energySum_R = energy_R[0]+energy_R[1]+energy_R[2]+energy_R[3];

	if(!useFilterEnergy){
		xpos_L = ((tqdc_L[0]+tqdc_L[1])-(tqdc_L[2]+tqdc_L[3]))/tqdcSum_L;
		ypos_L = ((tqdc_L[1]+tqdc_L[2])-(tqdc_L[0]+tqdc_L[3]))/tqdcSum_L;

		xpos_R = -((tqdc_R[0]+tqdc_R[1])-(tqdc_R[2]+tqdc_R[3]))/tqdcSum_R; // Sign is flipped to preserve x-axis of left side.
		ypos_R = ((tqdc_R[1]+tqdc_R[2])-(tqdc_R[0]+tqdc_R[3]))/tqdcSum_R;
	}
	else{
		xpos_L = ((energy_L[0]+energy_L[1])-(energy_L[2]+energy_L[3]))/energySum_L;
		ypos_L = ((energy_L[1]+energy_L[2])-(energy_L[0]+energy_L[3]))/energySum_L;

		xpos_R = -((energy_R[0]+energy_R[1])-(energy_R[2]+energy_R[3]))/energySum_R; // Sign is flipped to preserve x-axis of left side.
		ypos_R = ((energy_R[1]+energy_R[2])-(energy_R[0]+energy_R[3]))/energySum_R;	
	}	
}

void fullBarEvent::clear(){
	tdiff_L = 0;
	tdiff_R = 0;
	xpos_L = 0;
	xpos_R = 0;
	ypos_L = 0;
	ypos_R = 0;
	stqdc_L = 0;
	stqdc_R = 0;
	ltqdc_L = 0;
	ltqdc_R = 0;
	loc = 0;

	for(size_t i = 0; i < 4; i++){
		tqdc_L[i] = 0;
		tqdc_R[i] = 0;
		energy_L[i] = 0;
		energy_R[i] = 0;
	}
	
	tqdcSum_L = 0;
	tqdcSum_R = 0;
	energySum_L = 0;
	energySum_R = 0;

	valid_L = false;
	valid_R = false;
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

bool pspmtMapEntry::checkDynode(){
	if(mult[0] == 0) return false;
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
	TriggerStructure *sptr;

	bool singleEndedMode;
	bool noTimeMode;
	bool noEnergyMode;
	bool noPositionMode;
	bool calibrationMode;
	bool useLightBalance;

	std::string countsString;
	unsigned long long totalCounts;
	double totalDataTime;
	double userEnergyOffset;

	std::vector<pspmtPosCal> pspmtcal;
	std::vector<double> posCalX, posCalY;

	double x, y, z, r, theta, phi;
	double tdiff, tof, ctof, tqdc, stqdc, lbal, ctqdc, energy;
	double centerE, barifierE, xdetL, xdetR, ydetL, ydetR;
	unsigned short location;

	double cxdet, cydet;
	short xcell, ycell;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;
	float stqdc_L, stqdc_R;	
	float startqdc;

	float allTQDC_L[4];
	float allTQDC_R[4];

	void process();

	void handleCalibration();

	void handleEvents();

	void setVariables(fullEvent* evt_);

	void setVariables(fullBarEvent* evt_);

  public:
	pspmtHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), map(), barmap(), ptr(NULL), sptr(NULL), singleEndedMode(false), noTimeMode(false), 
	                 noEnergyMode(false), noPositionMode(false), calibrationMode(false), useLightBalance(false), totalCounts(0), totalDataTime(0), userEnergyOffset(0) { }

	~pspmtHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void pspmtHandler::process(){
	// Check for invalid TQDC.
	if(tqdc_L <= 0 || (!singleEndedMode && tqdc_R <= 0)) return;

	BarCal *bar = NULL;
	if(!singleEndedMode && !(bar = calib.GetBarCal(location))) return;

	TimeCal *time = NULL;
	double tDiffOffset_L = 0;
	double tDiffOffset_R = 0;
	if(!noTimeMode){
		if(!(time = calib.GetTimeCal(location))) return;
		tDiffOffset_L = time->t0;
	}
	if(!singleEndedMode){
		TimeCal *time_R = calib.GetTimeCal(location+1);
		if(time_R) tDiffOffset_R = time_R->t0;
	}

	PositionCal *pos = NULL;
	pspmtPosCal *pspmtpos = NULL;
	if(!noPositionMode){
		if(!(pos = calib.GetPositionCal(location)) || location >= pspmtcal.size()) return;
		pspmtpos = &pspmtcal.at(location);
	}

	// Compute the corrected time difference
	if(!singleEndedMode){
		tdiff = (tdiff_R - tdiff_L);
		lbal = (tqdc_L-tqdc_R)/(tqdc_L+tqdc_R);
		if(!useLightBalance){
			tdiff = tdiff - bar->t0;
			y = bar->cbar*tdiff/200; // m
		}
		else{ // Use light balance to compute position in detector.
			lbal = lbal - bar->t0;
			y = lbal*((bar->length/100)/bar->beta);
		}
	}
	else y = 0; // m

	// Compute the 3d position of the detection event
	if(!noPositionMode){
		// Get the calibrated X and Y positions.
		if(!singleEndedMode)
			pspmtpos->calibrate((xdetL+xdetR)/2, (ydetL+ydetR)/2, cxdet, cydet);
		else
			pspmtpos->calibrate(xdetL, ydetL, cxdet, cydet);

		// Get the X and Y pixel hit locations.
		xcell = getXcell(cxdet);
		ycell = getYcell(cydet);

		// Vector from the center to the interaction point.
		Vector3 p(cxdet, cydet, y); 
	
		// Rotate to the frame of the bar.
		pos->Transform(p);
	
		// Calculate the vector from the origin of the lab frame.
		Vector3 r0 = (*pos->GetPosition()) + p;
		x = r0.axis[0]; // m
		y = r0.axis[1]; // m
		z = r0.axis[2]; // m

		// Convert the event vector to spherical.
		Cart2Sphere(r0, r, theta, phi);
	}
	else{
		r = 0;
		theta = 0;
		phi = 0;
		x = 0;
		z = 0;
	}

	if(!singleEndedMode)
		tof = (tdiff_R - tDiffOffset_R + tdiff_L - tDiffOffset_L)/2;
	else
		tof = tdiff_L;

	if(userEnergyOffset > 0)
		tof += energy2tof(userEnergyOffset, pos->r0) - 100*pos->r0/cvac;

	// Calculate the corrected TOF.
	ctof = tof;
	//if(!noTimeMode) // Correct timing offset. This should place the gamma-flash at t=0 ns.
	//	ctof = ctof - time->t0;
	if(!noPositionMode) // Correct the gamma-flash offset for distance from source.
		ctof += 100*pos->r0/cvac;

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

	// Get the TQDC from the start detector.
	if(sptr && sptr->mult > 0)
		startqdc = sptr->tqdc.at(0);
	else
		startqdc = 0;

	// Calculate the neutron energy.
	if(!noPositionMode){
		energy = tof2energy(ctof, r);
		
		// Compute the energy to the center of the bar (i.e. no segmentation, equivalent to barifier).
		if(debug){ 
			// Select a random point inside the bar.
			double xdetRan = frand(-bar->width/200, bar->width/200);
			double ydetRan = frand(-bar->height/200, bar->height/200);

			// Compute the "non-segmented" random position in the bar.			
			Vector3 p(xdetRan, ydetRan, y);
			pos->Transform(p);
			Vector3 r0 = (*pos->GetPosition()) + p;
			
			// Compute the energy.
			barifierE = tof2energy(ctof, r0.Length());
			
			p = Vector3(0, 0, y);
			pos->Transform(p);
			r0 = (*pos->GetPosition()) + p;
			
			centerE = tof2energy(ctof, r0.Length());
		}
	}
	
	// Fill the tree with the event.
	outtree->Fill();
}

void pspmtHandler::handleCalibration(){
	if(posCalX.empty() || posCalY.empty()) return; 

	TH2F *calHist2d = new TH2F("calHist2d", "pspmt calibration hist", 200, -0.8, 0.8, 250, -0.8, 0.8);
	TH1F *calHistY = new TH1F("calHistY", "pspmt calibration hist for y-axis", 200, -0.8, 0.8);
	TH1F *calHistX[4];
	for(size_t i = 0; i < 4; i++){
		std::stringstream stream;
		stream << "calHistX" << i;
		calHistX[i] = new TH1F(stream.str().c_str(), "pspmt calibration hist for x-axis", 200, -0.8, 0.8);
	}

	std::vector<double>::iterator iterX;
	std::vector<double>::iterator iterY;
	for(iterX = posCalX.begin(), iterY = posCalY.begin(); iterX != posCalX.end() && iterY != posCalY.end(); iterX++, iterY++){
		calHist2d->Fill(*iterX, *iterY);
		calHistY->Fill(*iterY);
	}

	TSpectrum yspec(4);
	TSpectrum xspec(8);
	
	TF1 *fy = new TF1("fy", getFuncStr(4).c_str(), -0.8, 0.8);
	TF1 *fx = new TF1("fx", getFuncStr(8).c_str(), -0.8, 0.8);

	yspec.Search(calHistY);
	setInitPars(fy, &yspec, 4, 0.03);
	calHistY->Fit(fy, "QR");

	std::vector<gPar> ypars;
	std::vector<gPar> xpars[4];

	for(int i = 0; i < 4; i++){
		ypars.push_back(gPar(fy->GetParameter(3*i), fy->GetParameter(3*i+1), fy->GetParameter(3*i+2)));
		std::cout << " debug: sigma=" << fy->GetParameter(3*i+2) << std::endl;
	}
	std::cout << " debug: offset=" << fy->GetParameter(12) << std::endl;
	
	// Sort by mean. The fitter doesn't necessarily put them in ascending order.
	std::sort(ypars.begin(), ypars.end(), compare);

	// Open output calibration file.
	std::ofstream ofile("pspmtpos.cal");

	double yMinima[5] = {-0.8, 0, 0, 0, 0.8};
	
	for(int i = 1; i < 4; i++){
		yMinima[i] = fy->GetMinimumX(ypars[i-1].p1, ypars[i].p1);
		std::cout << " debug: y[" << i-1 << "] = " << yMinima[i] << std::endl;
	}
	ofile << yMinima[1] << "\t" << yMinima[2] << "\t" << yMinima[3];

	// Fill the x-position histograms.
	for(int i = 1; i <= 4; i++){
		for(iterX = posCalX.begin(), iterY = posCalY.begin(); iterX != posCalX.end() && iterY != posCalY.end(); iterX++, iterY++){
			if(*iterY > yMinima[i-1] && *iterY <= yMinima[i])
				calHistX[i-1]->Fill(*iterX);
		}
		xspec.Search(calHistX[i-1]);
		setInitPars(fx, &xspec, 8, 0.03);
		calHistX[i-1]->Fit(fx, "QR");
		for(int j = 0; j < 8; j++){
			xpars[i-1].push_back(gPar(fx->GetParameter(3*j), fx->GetParameter(3*j+1), fx->GetParameter(3*j+2)));
		}
		// Sort by mean.
		std::sort(xpars[i-1].begin(), xpars[i-1].end(), compare);
	}
	
	std::vector<TGraph*> xpoints;
	TGraph *ypoints = new TGraph(4);
	TGraph *allPoints = new TGraph(32);

	ypoints->SetMarkerStyle(21);
	allPoints->SetMarkerStyle(21);

	// Horizontal lines
	for(int i = 0; i < 4; i++){
		ypoints->SetPoint(i, ypars[i].p1, -halfHeight+ySpacing*(0.5+i));
	}

	std::vector<TF1*> xCalFit;
	TF1 *yCalFit = new TF1("yCalFit", "pol1", -0.8, 0.8);
	
	// Fit the y-position spectrum.
	yCalFit->SetParameters(0, 0.3);
	ypoints->Fit(yCalFit, "QR");

	for(int i = 0; i < 4; i++){ // Fit the x-position spectra.
		xpoints.push_back(new TGraph(8));
		xCalFit.push_back(new TF1("xCalFit", "pol3", -0.8, 0.8));
		for(int j = 0; j < 8; j++){
			xpoints.back()->SetPoint(j, xpars[i][j].p1, -halfWidth+xSpacing*(0.5+j));
			allPoints->SetPoint(i*8+j, xpars[i][j].p1, ypars[i].p1);
		}
		xCalFit.back()->SetParameters(0, 0.1, 0, 0);
		xpoints.back()->Fit(xCalFit.back(), "QR");
		ofile << "\t" << xCalFit.back()->GetParameter(0) << "\t" << xCalFit.back()->GetParameter(1) << "\t" << xCalFit.back()->GetParameter(2) << "\t" << xCalFit.back()->GetParameter(3);
	}

	ofile << "\t" << yCalFit->GetParameter(0) << "\t" << yCalFit->GetParameter(1) << std::endl;
	ofile.close();

	if(debug){ // Draw debug histograms to the screen.
		initRootGraphics();

		openCanvas1()->Divide(2, 2);

		for(int i = 0; i < 4; i++){
			can1->cd(i+1);
			xpoints.at(i)->SetMarkerStyle(21);
			xpoints.at(i)->Draw("AP");
			xCalFit.at(i)->Draw("SAME");
		}

		/*can1->cd();
		calHist2d->SetStats(0);
		calHist2d->Draw("COLZ");
		allPoints->Draw("PSAME");*/

		openCanvas2()->Divide(2, 2);

		for(int i = 0; i < 4; i++){
			can2->cd(i+1);
			calHistX[i]->Draw();
		}
		
		// Wait for the user to finish before closing the application.
		wait();
	}

	delete calHist2d;
	delete calHistY;
	for(int i = 0; i < 4; i++){
		delete calHistX[i];
	}
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

			// Check the PSD (if available).
			if(tcutg && !tcutg->IsInside(tqdc, stqdc/tqdc)) continue;

			if(!calibrationMode)
				process();
			else{
				posCalX.push_back((xdetL+xdetR)/2);
				posCalY.push_back((ydetL+ydetR)/2);
			}
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

			// Check the PSD (if available).
			if(tcutg && !tcutg->IsInside(tqdc, stqdc/tqdc)) continue;

			if(!calibrationMode)
				process();
			else{
				posCalX.push_back(xdetL);
				posCalY.push_back(ydetL);
			}
			delete (*iter);
		}
		fullEvents.clear();
		events.clear();
	}
}

void pspmtHandler::setVariables(fullEvent *evt_){
	tdiff_L = evt_->tdiff;
	tqdc_L = evt_->ltqdc;
	stqdc_L = evt_->stqdc;
	location = evt_->loc;

	xdetL = evt_->xpos;
	ydetL = evt_->ypos;

	// Calculate the long and short integrals for PSD.
	stqdc = stqdc_L;
	tqdc = tqdc_L;


	if(debug){
		for(size_t i = 0; i < 4; i++) allTQDC_L[i] = evt_->tqdc[i];
	}
}

void pspmtHandler::setVariables(fullBarEvent *evt_){
	tdiff_L = evt_->tdiff_L;
	tdiff_R = evt_->tdiff_R;
	tqdc_L = evt_->ltqdc_L;
	tqdc_R = evt_->ltqdc_R;
	stqdc_L = evt_->stqdc_L;
	stqdc_R = evt_->stqdc_R;
	location = evt_->loc;

	if(evt_->valid_L && evt_->valid_R){
		xdetL = evt_->xpos_L;
		ydetL = evt_->ypos_L;
		xdetR = evt_->xpos_R;
		ydetR = evt_->ypos_R;
	}
	else if(evt_->valid_L){
		xdetL = evt_->xpos_L;
		ydetL = evt_->ypos_L;
		xdetR = xdetL;
		ydetR = ydetL;
	}
	else if(evt_->valid_R){
		xdetR = evt_->xpos_R;
		ydetR = evt_->ypos_R;
		xdetL = xdetR;
		ydetL = ydetR;
	}
	else{ std::cout << "HERE!\n"; } // This should never happen.

	// Calculate the long and short integrals for PSD.
	stqdc = std::sqrt(stqdc_R*stqdc_L);
	tqdc = std::sqrt(tqdc_R*tqdc_L);

	if(debug){
		for(size_t i = 0; i < 4; i++){
			allTQDC_L[i] = evt_->tqdc_L[i];
			allTQDC_R[i] = evt_->tqdc_R[i];
		}
	}
}

pspmtHandler::~pspmtHandler(){
}

void pspmtHandler::addOptions(){
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read bar speed-of-light from an input cal file."), userOpts, optstr);
	addOption(optionExt("single", no_argument, NULL, 0x0, "", "Single-ended PSPMT detector mode."), userOpts, optstr);
	addOption(optionExt("debug", no_argument, NULL, 0x0, "", "Enable debug output."), userOpts, optstr);
	addOption(optionExt("light", no_argument, NULL, 0x0, "", "Use TQDC light-balance to compute Y instead of PMT tdiff."), userOpts, optstr);	
	addOption(optionExt("no-energy", no_argument, NULL, 0x0, "", "Do not use energy calibration."), userOpts, optstr);
	addOption(optionExt("no-time", no_argument, NULL, 0x0, "", "Do not use time calibration."), userOpts, optstr);
	addOption(optionExt("no-position", no_argument, NULL, 0x0, "", "Do not use position calibration."), userOpts, optstr);
	addOption(optionExt("filter-energy", no_argument, NULL, 0x0, "", "Use Pixie filter energy instead of TQDC."), userOpts, optstr);
	addOption(optionExt("calibrate", no_argument, NULL, 0x0, "", "Calibrate the PSPMT X-Y position."), userOpts, optstr);
	addOption(optionExt("energy-offset", required_argument, NULL, 0x0, "<energy>", "Set the neutron energy offset for data with no reference time."), userOpts, optstr);
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
		useLightBalance = true;
	}
	if(userOpts.at(4).active){
		noEnergyMode = true;
	}
	if(userOpts.at(5).active){
		noTimeMode = true;
	}
	if(userOpts.at(6).active){
		noPositionMode = true;
	}
	if(userOpts.at(7).active){
		useFilterEnergy = true;
	}
	if(userOpts.at(8).active){
		calibrationMode = true;
	}
	if(userOpts.at(9).active){
		userEnergyOffset = strtod(userOpts.at(9).argument.c_str(), NULL);
		std::cout << " Setting neutron energy offset to " << userEnergyOffset << " MeV.\n";
	}

	return true;
}

int pspmtHandler::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(!cut_filename.empty() && !loadTCutG())
		return 1;

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

	if(!calibrationMode){
		if(!noPositionMode){
			if(!calib.LoadPositionCal((setupDir+"position.cal").c_str())) return 3;
			else if(!LoadCalibFile((setupDir+"pspmtpos.cal").c_str(), pspmtcal)) return 3;
			for(size_t i = 0; i < pspmtcal.size(); i++)
				std::cout << pspmtcal.at(i).Print(true);
		}
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
		outtree->Branch("ltqdc", &tqdc);
		outtree->Branch("stqdc", &stqdc);
		if(!noEnergyMode)
			outtree->Branch("ctqdc", &ctqdc);
		if(!noPositionMode){
			outtree->Branch("r", &r);
			outtree->Branch("theta", &theta);
			outtree->Branch("phi", &phi);
			outtree->Branch("x", &x);
			outtree->Branch("y", &y);
			outtree->Branch("z", &z);
		}
		else if(!singleEndedMode)
			outtree->Branch("y", &y);
		if(debug){ // Diagnostic branches
			outtree->Branch("tof", &tof);
			outtree->Branch("lbal", &lbal);
			outtree->Branch("cenE", &centerE);
			outtree->Branch("barE", &barifierE);
			outtree->Branch("startqdc", &startqdc);
			if(!singleEndedMode){
				outtree->Branch("tdiff", &tdiff);
				outtree->Branch("xdetL", &xdetL);
				outtree->Branch("ydetL", &ydetL);
				outtree->Branch("xdetR", &xdetR);
				outtree->Branch("ydetR", &ydetR);
			}
			else{
				outtree->Branch("xdet", &xdetL);
				outtree->Branch("ydet", &ydetL);
			}
			outtree->Branch("cxdet", &cxdet);
			outtree->Branch("cydet", &cydet);
		}
		outtree->Branch("xcell", &xcell);
		outtree->Branch("ycell", &ycell);
		if(debug){
			if(!singleEndedMode){
				outtree->Branch("anodeL[4]", allTQDC_L);
				outtree->Branch("anodeR[4]", allTQDC_R);
			}
			else outtree->Branch("anode[4]", allTQDC_L);
		}
		outtree->Branch("loc", &location);
	}

	int file_counter = 1;
	while(openInputFile()){
		std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

		if(!loadInputTree()){ // Load the input tree.
			std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
			continue;
		}

		TBranch *branch = NULL, *sbranch = NULL;
		intree->SetBranchAddress("pspmt", &ptr, &branch);
		intree->SetBranchAddress("trigger", &sptr, &sbranch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"pspmt\" from input TTree.\n";
			return 9;
		}
		if(!sbranch){ // Not a fatal error.
			std::cout << " Warning! Failed to load branch \"trigger\" from input TTree.\n";
		}

		if(!calibrationMode){
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
		}

		// Handle all events.
		while(getNextEntry())
			handleEvents();
	}

	// Perform position calibration if needed.
	if(calibrationMode) handleCalibration();

	if(!calibrationMode){
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
	}
			
	return 0;
}

int main(int argc, char *argv[]){
	pspmtHandler obj;

	return obj.execute(argc, argv);
}
