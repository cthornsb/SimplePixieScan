#ifndef HAGRID_PROCESSOR_HPP
#define HAGRID_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;
class Plotter;

class HagridProcessor : public Processor{
  private:
	HagridStructure structure;
	Trace waveform;
  
	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_1d;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	HagridProcessor(MapFile *map_);

	~HagridProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
