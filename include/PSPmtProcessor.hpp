#ifndef PSPMT_PROCESSOR_HPP
#define PSPMT_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class PSPmtProcessor : public Processor{
  private:
	PSPmtStructure structure;
	Trace waveform;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PSPmtProcessor(MapFile *map_);

	~PSPmtProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
