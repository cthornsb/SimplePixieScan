#ifndef PSPMT_PROCESSOR_HPP
#define PSPMT_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class PSPmtEvent{
  public:
	float xpos;
	float ypos;
	float anodes[4];
	bool channels[4];

	PSPmtEvent(){ reset(); }

	void reset();

	bool addAnode(const float &anode, const size_t &index);

	bool allValuesSet();
};

class PSPmtMap{
  public:
	PSPmtMap();
	
	PSPmtMap(const int &dynodeL);
	
	PSPmtMap(const int &dynodeL, const int &dynodeR);

	int getLocation() const { return channels[0][0]; }

	bool getDoubleSided() const { return isDoubleSided; }
	
	PSPmtEvent *getEventL(){ return &pspmtEventL; }
	
	PSPmtEvent *getEventR(){ return &pspmtEventR; }
	
	bool check(const int &location, bool &isDynode, bool &isRight, unsigned short &tqdcIndex) const ;
	
	bool checkLocations() const ;

	void setDynodes(const int &dynode);

	void setDynodes(const int &dynodeL, const int &dynodeR);
	
	void setAnodes(const int &se, const int &ne, const int &nw, const int &sw);	
	
	void setAnodes(const int &seL, const int &neL, const int &nwL, const int &swL, const int &seR, const int &neR, const int &nwR, const int &swR);
	
	void setLocationByTag(const int &location, const std::string &subtype, const std::string &tag);
	
	bool setNextLocation(const int &location);
	
	void print() const ;
	
  private:
	bool isDoubleSided;
	
	size_t index1;
	size_t index2;

	PSPmtEvent pspmtEventL;
	PSPmtEvent pspmtEventR;

	int channels[2][5];
};

class PSPmtProcessor : public Processor{
  private:
	PSPmtStructure structure;
	Trace waveform;

	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_xpos_2d;
	Plotter *loc_ypos_2d;
	Plotter *ypos_xpos_2d;
	Plotter *sltqdc_ltqdc_2d;
	Plotter *loc_1d;

	std::vector<PSPmtMap> detMap;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PSPmtProcessor(MapFile *map_);

	~PSPmtProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);

	/** Manually add PSPmt detector locations to the online processor detector list
	  * @param locations Vector of all PSPmt locations defined in the map
	  * @return True
	  */
	virtual bool AddDetectorLocations(std::vector<int> &locations);

	virtual void Reset();
};

#endif
