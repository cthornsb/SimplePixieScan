#ifndef GENERICBAR_PROCESSOR_HPP
#define GENERICBAR_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class GenericBarProcessor : public Processor{
  private:
	GenericBarStructure structure;
	Trace L_waveform;
	Trace R_waveform;

	Plotter *tdiff_1d;
	Plotter *tof_1d;
	Plotter *tqdc_1d;
	Plotter *tqdc_tof_2d;
	Plotter *maxADC_tof_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	GenericBarProcessor(MapFile *map_);

	~GenericBarProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
