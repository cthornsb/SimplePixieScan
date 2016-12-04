#ifndef LIQUIDBAR_PROCESSOR_HPP
#define LIQUIDBAR_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class LiquidBarProcessor : public Processor{
  private:
	LiquidBarStructure structure;
	Trace L_waveform;
	Trace R_waveform;

	double left_short_qdc; /// The integral of the short portion of the left pmt pulse.
	double left_long_qdc; /// The integral of the long portion of the left pmt pulse.
	double right_short_qdc; /// The integral of the short portion of the right pmt pulse.
	double right_long_qdc; /// The integral of the long portion of the right pmt pulse.

	Plotter *loc_tdiff_2d;
	Plotter *loc_short_energy_2d;
	Plotter *loc_long_energy_2d;
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
