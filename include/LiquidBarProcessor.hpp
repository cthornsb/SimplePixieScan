#ifndef LIQUIDBAR_PROCESSOR_HPP
#define LIQUIDBAR_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class LiquidBarProcessor : public Processor{
  private:
	LiquidBarStructure structure;
	LiquidBarWaveform waveform;

	double short_qdc; /// The integral of the short portion of the pulse.
	double long_qdc; /// The integral of the long portion of the pulse.

	int fitting_low2;
	int fitting_high2;

	Plotter *loc_tdiff_2d;
	Plotter *loc_short_energy_2d;
	Plotter *loc_long_energy_2d;
	Plotter *loc_psd_2d;
	Plotter *loc_1d;

	/// Set the CFD parameters for the current event.
	virtual bool SetCfdParameters(ChannelEvent *event_, MapEntry *entry_);

	/// Process all individual events.
	virtual bool HandleEvents();
	
  public:
	LiquidBarProcessor(MapFile *map_);
	
	~LiquidBarProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
