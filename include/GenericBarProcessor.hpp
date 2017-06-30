#ifndef GENERICBAR_PROCESSOR_HPP
#define GENERICBAR_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class GenericBarProcessor : public Processor{
  private:
	GenericBarStructure structure;
	Trace L_waveform;
	Trace R_waveform;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	GenericBarProcessor(MapFile *map_);

	~GenericBarProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
