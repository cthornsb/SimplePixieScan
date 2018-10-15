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

class PSPmtProcessor : public Processor{
  private:
	PSPmtStructure structure;
	Trace waveform;

	PSPmtEvent pspmtEventL;
	PSPmtEvent pspmtEventR;

	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_xpos_2d;
	Plotter *loc_ypos_2d;
	Plotter *ypos_xpos_2d;
	Plotter *sltqdc_ltqdc_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PSPmtProcessor(MapFile *map_);

	~PSPmtProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);

	virtual void Reset();
};

#endif
