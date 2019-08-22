#ifndef LIQUID_BAR_PROCESSOR_HPP
#define LIQUID_BAR_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class LiquidBarProcessor : public Processor{
  private:
	LiquidBarStructure structure;
	Trace waveform;

	float short_qdc; /// The integral of the short portion of the left pmt pulse.
	float long_qdc; /// The integral of the long portion of the left pmt pulse.

	Plotter *tdiff_1d;
	Plotter *tof_1d;
	Plotter *long_1d;
	Plotter *psd_long_2d;
	Plotter *long_tof_2d;
	Plotter *maxADC_tof_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	LiquidBarProcessor(MapFile *map_);
	
	~LiquidBarProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
