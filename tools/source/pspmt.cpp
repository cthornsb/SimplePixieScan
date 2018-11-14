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

std::string getFuncStr(int nPeak){
	std::stringstream retval;
	retval << "gaus";
	for(int i = 1; i < nPeak; i++)
		retval << "+gaus(" << 3*i << ")";
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

double pspmtPosCal::calX(const double &x0_){
	return (xp[0] + xp[1]*x0_ + xp[2]*x0_*x0_ + xp[3]*x0_*x0_*x0_);
}

double pspmtPosCal::calY(const double &y0_){
	return (yp[0] + yp[1]*y0_ + yp[2]*y0_*y0_ + yp[3]*y0_*y0_*y0_);
}

std::string pspmtPosCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy){
		output << " id=" << id << ", x=(" << xp[0] << ", " << xp[1] << ", " << xp[2] << ", " << xp[3] << ")";
		output << ", y=(" << yp[0] << ", " << yp[1] << ", " << yp[2] << ", " << yp[3] << ")";
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
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else if(index <= 4) // x parameters
			xp[index-1] = strtod(iter->c_str(), NULL);
		else if(index <= 8) // y parameters
			yp[index-5] = strtod(iter->c_str(), NULL);
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
	
fullBarEvent::fullBarEvent(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
                           simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R){
	compute(dynode_L, anode_SE_L, anode_NE_L, anode_NW_L, anode_SW_L, dynode_R, anode_SE_R, anode_NE_R, anode_NW_R, anode_SW_R);
}

void fullBarEvent::compute(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
                           simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R){
	tdiff_L = dynode_L->tdiff;
	tdiff_R = dynode_R->tdiff;

	ltqdc_L = dynode_L->ltqdc;
	ltqdc_R = dynode_R->ltqdc;
	stqdc_L = dynode_L->stqdc;
	stqdc_R = dynode_R->stqdc;
	
	tqdc_L[0] = anode_SE_L->ltqdc;
	tqdc_L[1] = anode_NE_L->ltqdc;
	tqdc_L[2] = anode_NW_L->ltqdc;
	tqdc_L[3] = anode_SW_L->ltqdc;

	tqdc_R[0] = anode_SE_R->ltqdc;
	tqdc_R[1] = anode_NE_R->ltqdc;
	tqdc_R[2] = anode_NW_R->ltqdc;
	tqdc_R[3] = anode_SW_R->ltqdc;

	energy_L[0] = (float)anode_SE_L->energy;
	energy_L[1] = (float)anode_NE_L->energy;
	energy_L[2] = (float)anode_NW_L->energy;
	energy_L[3] = (float)anode_SW_L->energy;

	energy_R[0] = (float)anode_SE_R->energy;
	energy_R[1] = (float)anode_NE_R->energy;
	energy_R[2] = (float)anode_NW_R->energy;
	energy_R[3] = (float)anode_SW_R->energy;

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

	TH2F *calHist;

	bool singleEndedMode;
	bool noTimeMode;
	bool noEnergyMode;
	bool noPositionMode;
	bool calibrationMode;

	std::string countsString;
	unsigned long long totalCounts;
	double totalDataTime;

	std::vector<pspmtPosCal> pspmtcal;

	double x, y, z, r, theta, phi;
	double tdiff, tof, ctof, tqdc, stqdc, lbal, ctqdc, energy;
	double xdetL, xdetR, ydetL, ydetR;
	unsigned short location;

	double cxdet, cydet;
	short xcell, ycell;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;
	float stqdc_L, stqdc_R;	

	float allTQDC_L[4];
	float allTQDC_R[4];

	void process();

	void handleCalibration();

	void handleEvents();

	void setVariables(fullEvent* evt_);

	void setVariables(fullBarEvent* evt_);

  public:
	pspmtHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), map(), barmap(), ptr(NULL), calHist(NULL), singleEndedMode(false), 
	                 noTimeMode(false), noEnergyMode(false), noPositionMode(false), calibrationMode(false), totalCounts(0), totalDataTime(0) { }

	~pspmtHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void pspmtHandler::process(){
	//double ctdiff;

	// Check for invalid TQDC.
	if(tqdc_L <= 0 || (!singleEndedMode && tqdc_R <= 0)) return;

	BarCal *bar = NULL;
	if(!singleEndedMode && !(bar = calib.GetBarCal(location))) return;

	TimeCal *time = NULL;
	if(!noTimeMode && !(time = calib.GetTimeCal(location))) return;

	PositionCal *pos = NULL;
	pspmtPosCal *pspmtpos = NULL;
	if(!noPositionMode){
		if(!(pos = calib.GetPositionCal(location)) || location >= pspmtcal.size()) return;
		pspmtpos = &pspmtcal.at(location);
	}

	// Compute the corrected time difference
	if(!singleEndedMode){
		/*ctdiff = tdiff_R - tdiff_L - bar->t0;
		y = bar->cbar*ctdiff/200; // m

		// Check for invalid radius.
		if(y < -bar->length/30.0 || y > bar->length/30.0) return;*/

		// Calculate the light balance.
		lbal = (tqdc_L-tqdc_R)/(tqdc_L+tqdc_R) - bar->t0;

		// Use light balance to compute position in detector.
		y = lbal*((bar->length/100)/bar->beta);
	}
	else y = 0; // m

	// Compute the 3d position of the detection event
	if(!noPositionMode){
		// Get the calibrated X and Y positions.
		cxdet = pspmtpos->calX(xdetL);
		cydet = pspmtpos->calY(ydetL);

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

	if(!singleEndedMode){
		// Calculate the corrected TOF.
		tdiff = (tdiff_R - tdiff_L);
		tof = (tdiff_R + tdiff_L)/2;
		if(!noPositionMode){
			if(!noTimeMode)
				ctof = (pos->r0/r)*(tof - time->t0) + 100*pos->r0/cvac;
			else
				ctof = (pos->r0/r)*tof + 100*pos->r0/cvac;
		}
		else if(!noTimeMode)
			ctof = tof - time->t0;
		else
			ctof = tof;

		// Calculate the TQDC.
		tqdc = std::sqrt(tqdc_R*tqdc_L);
	}
	else{
		tof = tdiff_L;
		if(!noPositionMode){
			if(!noTimeMode)
				ctof = tof - time->t0 + 100*pos->r0/cvac;
			else
				ctof = tof + 100*pos->r0/cvac;
		}
		else if(!noTimeMode)
			ctof = tof - time->t0;
		else
			ctof = tof;
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

void pspmtHandler::handleCalibration(){
	TSpectrum yspec(4);
	TSpectrum xspec(8);
	
	TF1 *fy = new TF1("fy", getFuncStr(4).c_str(), -1, 1);
	TF1 *fx = new TF1("fx", getFuncStr(8).c_str(), -1, 1);

	TH1D *hx = calHist->ProjectionX("hx");
	TH1D *hy = calHist->ProjectionY("hy");

	yspec.Search(hy);
	xspec.Search(hx);

	setInitPars(fy, &yspec, 4);
	setInitPars(fx, &xspec, 8);
	
	hy->Fit(fy, "Q");
	hx->Fit(fx, "Q");

	std::vector<gPar> ypars;
	std::vector<gPar> xpars;

	for(int i = 0; i < 4; i++){
		ypars.push_back(gPar(fy->GetParameter(3*i), fy->GetParameter(3*i+1), fy->GetParameter(3*i+2)));
	}
	for(int i = 0; i < 8; i++){
		xpars.push_back(gPar(fx->GetParameter(3*i), fx->GetParameter(3*i+1), fx->GetParameter(3*i+2)));
	}
	
	// Sort by mean.
	std::sort(ypars.begin(), ypars.end(), compare);
	std::sort(xpars.begin(), xpars.end(), compare);

	double ypos[5];
	double xpos[9];
	
	for(int i = 1; i < 4; i++){
		ypos[i] = (ypars[i].p1 - ypars[i-1].p1)/2 + ypars[i-1].p1;
	}
	ypos[0] = ypars[0].p1 - (ypos[1]-ypars[0].p1);
	ypos[4] = ypars[3].p1 + (ypars[3].p1-ypos[3]);
	for(int i = 1; i < 8; i++){
		xpos[i] = (xpars[i].p1 - xpars[i-1].p1)/2 + xpars[i-1].p1;
	}
	xpos[0] = xpars[0].p1 - (xpos[1]-xpars[0].p1);
	xpos[8] = xpars[7].p1 + (xpars[7].p1-xpos[7]);
	
	TLine *ylines[5];
	TLine *xlines[9];

	TGraph *ypoints = new TGraph(5);
	TGraph *xpoints = new TGraph(9);

	xpoints->SetMarkerStyle(21);
	ypoints->SetMarkerStyle(21);

	// Horizontal lines
	for(int i = 0; i <= 4; i++){
		ypoints->SetPoint(i, ypos[i], -(height/2)+(height/4)*i);
		if(debug){
			ylines[i] = new TLine(xpos[0], ypos[i], xpos[8], ypos[i]);
			ylines[i]->SetLineWidth(2);
		}
	}
	// Vertical lines
	for(int i = 0; i <= 8; i++){
		xpoints->SetPoint(i, xpos[i], -(width/2)+(width/8)*i);
		if(debug){
			xlines[i] = new TLine(xpos[i], ypos[0], xpos[i], ypos[4]);
			xlines[i]->SetLineWidth(2);
		}
	}

	TF1 *xCalFit = new TF1("xCalFit", "pol3", -1, 1);
	TF1 *yCalFit = new TF1("yCalFit", "pol1", -1, 1);
	
	xCalFit->SetParameters(0, 3, 0, 0);
	yCalFit->SetParameters(0, 3);

	xpoints->Fit(xCalFit, "Q");
	ypoints->Fit(yCalFit, "Q");

	std::ofstream ofile("pspmtpos.cal");
	ofile << xCalFit->GetParameter(0) << "\t" << xCalFit->GetParameter(1) << "\t" << xCalFit->GetParameter(2) << "\t" << xCalFit->GetParameter(3);
	ofile << "\t" << yCalFit->GetParameter(0) << "\t" << yCalFit->GetParameter(1) << std::endl;
	ofile.close();

	if(debug){ // Draw debug histograms to the screen.
		std::cout << " PSPMT position calibration data:\n";
		std::cout << "  X: " << xCalFit->GetParameter(0) << "\t" << xCalFit->GetParameter(1) << "\t" << xCalFit->GetParameter(2) << "\t" << xCalFit->GetParameter(3) << std::endl;
		std::cout << "  Y: " << yCalFit->GetParameter(0) << "\t" << xCalFit->GetParameter(1) << std::endl;

		initRootGraphics();

		openCanvas1();

		can1->cd();
		calHist->SetStats(0);
		calHist->Draw("COLZ");
		for(int i = 0; i <= 4; i++){
			ylines[i]->Draw("SAME");
		}
		for(int i = 0; i <= 8; i++){
			xlines[i]->Draw("SAME");
		}

		openCanvas2()->Divide(2, 2);

		can2->cd(1);
		hx->Draw();
		can2->cd(2);
		hy->Draw();

		can2->cd(3);
		xpoints->Draw("AP");
		can2->cd(4);
		ypoints->Draw("AP");
		
		// Wait for the user to finish before closing the application.
		wait();
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
			if(!calibrationMode)
				process();
			else
				calHist->Fill(xdetL, ydetL);
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
			if(!calibrationMode)
				process();
			else
				calHist->Fill(xdetL, ydetL);
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

	xdetL = evt_->xpos_L;
	ydetL = evt_->ypos_L;
	xdetR = evt_->xpos_R;
	ydetR = evt_->ypos_R;

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
	addOption(optionExt("no-energy", no_argument, NULL, 0x0, "", "Do not use energy calibration."), userOpts, optstr);
	addOption(optionExt("no-time", no_argument, NULL, 0x0, "", "Do not use time calibration."), userOpts, optstr);
	addOption(optionExt("no-position", no_argument, NULL, 0x0, "", "Do not use position calibration."), userOpts, optstr);
	addOption(optionExt("filter-energy", no_argument, NULL, 0x0, "", "Use Pixie filter energy instead of TQDC."), userOpts, optstr);
	addOption(optionExt("calibrate", no_argument, NULL, 0x0, "", "Calibrate the PSPMT X-Y position."), userOpts, optstr);
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
	if(userOpts.at(6).active){
		useFilterEnergy = true;
	}
	if(userOpts.at(7).active){
		calibrationMode = true;
		calHist = new TH2F("calHist", "pspmt calibration hist", 250, -1, 1, 250, -1, 1);
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
		outtree->Branch("tqdc", &tqdc);
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
			outtree->Branch("xcell", &xcell);
			outtree->Branch("ycell", &ycell);
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

		TBranch *branch = NULL;
		intree->SetBranchAddress("pspmt", &ptr, &branch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"pspmt\" from input TTree.\n";
			return 9;
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
