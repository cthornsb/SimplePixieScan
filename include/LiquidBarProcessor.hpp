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

	Plotter *loc_tdiff_2d;
	Plotter *loc_short_tqdc_2d;
	Plotter *loc_long_tqdc_2d;
	Plotter *loc_psd_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	LiquidBarProcessor(MapFile *map_);
	
	~LiquidBarProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
