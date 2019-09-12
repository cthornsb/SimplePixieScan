#ifndef TRACE_PROCESSOR_HPP
#define TRACE_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class TraceProcessor : public Processor{
  private:
	TraceStructure structure;
	Trace waveform;

	Plotter *tof_1d;
	Plotter *tqdc_1d;
	Plotter *filter_1d;
	Plotter *maximum_1d;
	Plotter *maxADC_1d;
	Plotter *stddev_1d;
	Plotter *baseline_1d;
	Plotter *phase_1d;
	Plotter *tqdc_tof_2d;
	Plotter *maxADC_tof_2d;
	Plotter *phase_phase_2d;
	Plotter *loc_1d;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	TraceProcessor(MapFile *map_);

	~TraceProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
