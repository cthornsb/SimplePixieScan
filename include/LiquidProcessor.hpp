#ifndef LIQUID_PROCESSOR_HPP
#define LIQUID_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class LiquidProcessor : public Processor{
  private:
	LiquidStructure structure;
	Trace waveform;

	float short_qdc; /// The integral of the short portion of the left pmt pulse.
	float long_qdc; /// The integral of the long portion of the left pmt pulse.

	Plotter *loc_tdiff_2d;
	Plotter *loc_short_energy_2d;
	Plotter *loc_long_energy_2d;
	Plotter *loc_psd_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	LiquidProcessor(MapFile *map_);
	
	~LiquidProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
