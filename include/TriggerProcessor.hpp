#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;
class Plotter;

class TriggerProcessor : public Processor{
  private:
	TriggerStructure structure;
	Trace waveform;

	Plotter *energy_1d;
	Plotter *phase_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	TriggerProcessor(MapFile *map_);

	~TriggerProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
