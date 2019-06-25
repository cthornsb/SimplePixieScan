#ifndef PSPMT_HPP
#define PSPMT_HPP

#include <vector>
#include <string>
#include <deque>

#include "CalibFile.hpp"
#include "simpleTool.hpp"
#include "PSPmtMap.hpp"

class PSPmtStructure;

class gPar{
  public:
	double p0, p1, p2;
  
	gPar(){ }
	
	gPar(const double &A, const double &mean, const double &sigma) : p0(A), p1(mean), p2(sigma) { }
};

class pspmtPosCal : public CalType {
  public:
	double xp[4][4];
	double yp[4];
	double ymin[3];

	pspmtPosCal() : CalType(0) { 
		for(size_t i = 0; i < 4; i++){
			for(size_t j = 0; j < 4; j++)
				xp[i][j] = (j != 1 ? 0 : 1);
			yp[i] = (i != 1 ? 0 : 1);
			if(i < 3) ymin[i] = 0;
		}
	}

	double calX(const double &x0_, double *par_);

	double calY(const double &y0_);

	void calibrate(const double &x0_, const double &y0_, double &x1, double &y1);

	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);
};

class simpleEvent{
  public:
	double tdiff;
	float ltqdc;
	float stqdc;
	
	unsigned short location;
	unsigned short tqdcIndex;
	unsigned short energy;

	bool isBarDet;
	bool isRightEnd;
	bool isDynode;
	
	simpleEvent() : tdiff(0), ltqdc(0), stqdc(0), location(0), tqdcIndex(0), energy(0), isBarDet(false), isRightEnd(false), isDynode(false) { }
	
	simpleEvent(PSPmtStructure *ptr, const size_t &index);
};

class fullEvent{
  public:
	double tdiff; // Dynode time difference
	float xpos; // X-position computed from the tqdc
	float ypos; // Y-position computed from the tqdc
	float stqdc; // Short integral from the dynode signal.
	float ltqdc; // Long integral from the dynode signal.
	unsigned short loc; // Location of the detector
	
	float tqdc[4];
	float energy[4];	

	float tqdcSum;
	float energySum;

	fullEvent() : tdiff(0), xpos(0), ypos(0), stqdc(0), ltqdc(0), loc(0) { }
	
	fullEvent(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);
	
	void compute(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);
};
	
class fullBarEvent{
  public:
	double tdiff_L, tdiff_R; // Dynode time difference
	float xpos_L, xpos_R; // X-position computed from the tqdc
	float ypos_L, ypos_R; // Y-position computed from the tqdc
	float stqdc_L, stqdc_R; // Short integral from the dynode signal.
	float ltqdc_L, ltqdc_R; // Long integral from the dynode signal.
	unsigned short loc; // Location of the detector
	
	float tqdc_L[4];
	float energy_L[4];
	
	float tqdc_R[4];
	float energy_R[4];

	float tqdcSum_L, tqdcSum_R;
	float energySum_L, energySum_R;
	
	bool valid_L, valid_R;

	fullBarEvent(){ this->clear(); }

	fullBarEvent(std::deque<simpleEvent*> *left, std::deque<simpleEvent*> *right);

	fullBarEvent(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
	             simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R);

	bool readDynodes(simpleEvent *dynode_L, simpleEvent *dynode_R);

	bool readLeftAnodes(simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);
	
	bool readRightAnodes(simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);

	bool readAnode(float &tqdc, float &energy, simpleEvent *anode);

	void compute();

	void clear();
};

class pspmtMapFileEntry : public CalType {
  public:
	unsigned short an1, an2, an3, an4;
	
	pspmtMapFileEntry() : CalType(), an1(0), an2(0), an3(0), an4(0) { }
	
	pspmtMapFileEntry(const int &id_) : CalType(id_), an1(0), an2(0), an3(0), an4(0)  { }

	pspmtMapFileEntry(const int &id_, const unsigned short &an1_, const unsigned short &an2_, const unsigned short &an3_, const unsigned short &an4_) : 
		CalType(id_), an1(an1_), an2(an1_), an3(an1_), an4(an1_) { }
	
	pspmtMapFileEntry(const std::vector<std::string> &pars_);
	
	virtual std::string Print(bool fancy=true);

	virtual unsigned int ReadPars(const std::vector<std::string> &pars_);

	void GetIDs(unsigned short *arr);
};

class pspmtMapEntry{
  private:
	unsigned short mult[5];
	unsigned short ids[5];
	std::deque<simpleEvent*> events[5];

  public:
	pspmtMapEntry();
	
	pspmtMapEntry(int *ptr_);

	unsigned short getDynodeID() const { return ids[0]; }
	
	std::deque<simpleEvent*>* getEvents(){ return events; }

	void clear();
	
	bool add(simpleEvent *evt);

	bool check();

	bool checkDynode();
	
	fullEvent* buildEvent();
};

class pspmtMap{
  private:
	std::vector<pspmtMapEntry> entries;

  public:	
	pspmtMap() { }

	std::vector<pspmtMapEntry>::iterator getBegin(){ return entries.begin(); }

	std::vector<pspmtMapEntry>::iterator getEnd(){ return entries.end(); }

	size_t getLength(){ return entries.size(); }
	
	void addEntry(int *ptr_);
	
	bool addEvent(simpleEvent *evt_);
	
	void clear();
	
	void buildEventList(std::vector<fullEvent*>& vec_);
};

class pspmtBarMap{
  private:
	pspmtMap mapL, mapR;

  public:
	pspmtBarMap() : mapL(), mapR() { }

	void addEntry(int *ptrL_, int *ptrR_);

	void addLeftEntry(int *ptr_);

	void addRightEntry(int *ptr_);

	bool addEvent(simpleEvent *evtL_, simpleEvent *evtR_);

	bool addLeftEvent(simpleEvent *evt_);

	bool addRightEvent(simpleEvent *evt_);
	
	void clear();
	
	void buildEventList(std::vector<fullBarEvent*>& vec_);
};

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
	bool manualCalMode;
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

	std::vector<PSPmtMap> detMap; ///< List of PSPmt detectors read in from the user map file

	void process();

	void handleCalibration();

	void handleEvents();

	void setVariables(fullEvent* evt_);

	void setVariables(fullBarEvent* evt_);

  public:
	pspmtHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), map(), barmap(), ptr(NULL), sptr(NULL), singleEndedMode(true), noTimeMode(false), 
	                 noEnergyMode(false), noPositionMode(false), calibrationMode(false), manualCalMode(false), useLightBalance(false), totalCounts(0), totalDataTime(0), userEnergyOffset(0) { }

	~pspmtHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

#endif
