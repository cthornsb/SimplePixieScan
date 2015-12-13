#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;
class Plotter;

class TriggerProcessor : public Processor{
  private:
	TriggerStructure structure;
	TriggerWaveform waveform;

	Plotter *loc_energy_2d;
	Plotter *loc_phase_2d;
	Plotter *loc_1d;

	virtual bool HandleEvents();
	
  public:
	TriggerProcessor(MapFile *map_);

	~TriggerProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
