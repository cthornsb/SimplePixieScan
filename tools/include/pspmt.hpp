#ifndef PSPMT_HPP
#define PSPMT_HPP

#include <vector>
#include <string>
#include <deque>

class PSPmtStructure;

class simpleEvent{
  public:
	double tdiff;
	float ltqdc;
	float stqdc;
	
	unsigned short location;
	unsigned short tqdcIndex;

	bool isBarDet;
	bool isRightEnd;
	
	simpleEvent() : tdiff(0), ltqdc(0), stqdc(0), location(0), tqdcIndex(0), isBarDet(0), isRightEnd(0) { }
	
	simpleEvent(PSPmtStructure *ptr, const size_t &index);
};

class fullEvent{
  public:
	double tdiff; // Dynode time difference
	float xpos; // X-position computed from the tqdc
	float ypos; // Y-position computed from the tqdc
	unsigned short loc; // Location of the detector
	
	float ltqdc[4];
	float stqdc[4];
	
	float ltqdcSum;
	float stqdcSum;

	fullEvent() : tdiff(0), xpos(0), ypos(0), loc(0) { }
	
	fullEvent(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);
	
	void compute(simpleEvent *dynode, simpleEvent *anode_SE, simpleEvent *anode_NE, simpleEvent *anode_NW, simpleEvent *anode_SW);
};
	
class fullBarEvent{
  public:
	double tdiff_L, tdiff_R; // Dynode time difference
	float xpos_L, xpos_R; // X-position computed from the tqdc
	float ypos_L, ypos_R; // Y-position computed from the tqdc
	unsigned short loc; // Location of the detector
	
	float ltqdc_L[4];
	float ltqdc_R[4];

	float stqdc_L[4];
	float stqdc_R[4];

	float ltqdcSum_L, ltqdcSum_R;
	float stqdcSum_L, stqdcSum_R;
	
	fullBarEvent() : tdiff_L(0), tdiff_R(0), xpos_L(0), xpos_R(0), ypos_L(0), ypos_R(0), loc(0) { }
	
	fullBarEvent(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
	      simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R);
	
	void compute(simpleEvent *dynode_L, simpleEvent *anode_SE_L, simpleEvent *anode_NE_L, simpleEvent *anode_NW_L, simpleEvent *anode_SW_L,
	             simpleEvent *dynode_R, simpleEvent *anode_SE_R, simpleEvent *anode_NE_R, simpleEvent *anode_NW_R, simpleEvent *anode_SW_R);
};

class pspmtMapEntry{
  private:
	unsigned short mult[5];
	unsigned short ids[5];
	std::deque<simpleEvent*> events[5];

  public:
	pspmtMapEntry();
	
	pspmtMapEntry(const std::string &str_);

	unsigned short getDynodeID() const { return ids[0]; }
	
	std::deque<simpleEvent*>* getEvents(){ return events; }

	void clear();
	
	bool add(simpleEvent *evt);

	bool check();
	
	fullEvent* buildEvent();
};

fullBarEvent* buildBarEvent(pspmtMapEntry *entryL_, pspmtMapEntry *entryR_);

class pspmtMap{
  private:
	std::vector<pspmtMapEntry> entries;

  public:	
	pspmtMap() { }

	std::vector<pspmtMapEntry>::iterator getBegin(){ return entries.begin(); }

	std::vector<pspmtMapEntry>::iterator getEnd(){ return entries.end(); }

	size_t getLength(){ return entries.size(); }
	
	void addEntry(const std::string &str_);
	
	bool addEvent(simpleEvent *evt_);
	
	void clear();
	
	void buildEventList(std::vector<fullEvent*>& vec_);
};

class pspmtBarMap{
  private:
	pspmtMap mapL, mapR;

  public:
	pspmtBarMap() : mapL(), mapR() { }

	void addEntry(const std::string &strL_, const std::string &strR_);

	void addLeftEntry(const std::string &str_);

	void addRightEntry(const std::string &str_);

	bool addEvent(simpleEvent *evtL_, simpleEvent *evtR_);

	bool addLeftEvent(simpleEvent *evt_);

	bool addRightEvent(simpleEvent *evt_);
	
	void clear();
	
	void buildEventList(std::vector<fullBarEvent*>& vec_);
};

#endif
