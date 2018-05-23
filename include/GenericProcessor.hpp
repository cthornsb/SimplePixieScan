#ifndef GENERIC_PROCESSOR_HPP
#define GENERIC_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;
class Plotter;

class GenericProcessor : public Processor{
  private:
	GenericStructure structure;
	Trace waveform;
  
	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_phase_2d;
	Plotter *loc_1d;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	GenericProcessor(MapFile *map_);

	~GenericProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
