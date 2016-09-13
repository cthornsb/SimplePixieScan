#ifndef TRACE_PROCESSOR_HPP
#define TRACE_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class TraceProcessor : public Processor{
  private:
	TraceStructure structure;
	Trace waveform;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	TraceProcessor(MapFile *map_);

	~TraceProcessor(){ }

	virtual void GetHists(std::vector<Plotter*> &plots_){ }
};

#endif
