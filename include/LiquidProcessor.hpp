#ifndef LIQUID_PROCESSOR_HPP
#define LIQUID_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class LiquidProcessor : public Processor{
  private:
	LiquidStructure structure;
	Wave<int> waveform;

	double short_qdc; /// The integral of the short portion of the left pmt pulse.
	double long_qdc; /// The integral of the long portion of the left pmt pulse.

	int fitting_low2; /// Lower limit of the long fitting integral (in adc clock ticks).
	int fitting_high2; /// Upper limit of the long fitting integral (in adc clock ticks).

	Plotter *loc_tdiff_2d;
	Plotter *loc_short_energy_2d;
	Plotter *loc_long_energy_2d;
	Plotter *loc_psd_2d;
	Plotter *loc_1d;

	/// Process all individual events.
	virtual bool HandleEvents();
	
  public:
	LiquidProcessor(MapFile *map_);
	
	~LiquidProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
