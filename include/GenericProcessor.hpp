#ifndef GENERIC_PROCESSOR_HPP
#define GENERIC_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;
class Plotter;

class GenericProcessor : public Processor{
  private:
	GenericStructure structure;
	Trace waveform;
  
	Plotter *tof_1d;
	Plotter *tqdc_1d;
	Plotter *long_tof_2d;
	Plotter *maxADC_tof_2d;
	Plotter *loc_1d;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	GenericProcessor(MapFile *map_);

	~GenericProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
