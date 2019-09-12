#ifndef LIQUID_PROCESSOR_HPP
#define LIQUID_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class LiquidProcessor : public Processor{
  private:
	LiquidStructure structure;
	Trace waveform;

	float short_qdc; /// The integral of the short portion of the left pmt pulse.
	float long_qdc; /// The integral of the long portion of the left pmt pulse.

	Plotter *tof_1d;
	Plotter *long_1d;
	Plotter *psd_long_2d;
	Plotter *long_tof_2d;
	Plotter *maxADC_tof_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	LiquidProcessor(MapFile *map_);
	
	~LiquidProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
