#ifndef PSPMT_PROCESSOR_HPP
#define PSPMT_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class PSPmtProcessor : public Processor{
  private:
	PSPmtStructure structure;
	Trace waveform;

	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PSPmtProcessor(MapFile *map_);

	~PSPmtProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
