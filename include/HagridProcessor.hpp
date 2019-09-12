#ifndef HAGRID_PROCESSOR_HPP
#define HAGRID_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;
class Plotter;

class HagridProcessor : public Processor{
  private:
	HagridStructure structure;
	Trace waveform;
  
	Plotter *tof_1d;
	Plotter *tqdc_1d;
	Plotter *filter_1d;
	Plotter *maxADC_1d;
	Plotter *loc_1d;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	HagridProcessor(MapFile *map_);

	~HagridProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
