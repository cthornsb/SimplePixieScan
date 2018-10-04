#ifndef TRACE_PROCESSOR_HPP
#define TRACE_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class TraceProcessor : public Processor{
  private:
	TraceStructure structure;
	Trace waveform;

	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_maximum_2d;
	Plotter *loc_phase_2d;
	Plotter *loc_baseline_2d;
	Plotter *loc_1d;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	TraceProcessor(MapFile *map_);

	~TraceProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
