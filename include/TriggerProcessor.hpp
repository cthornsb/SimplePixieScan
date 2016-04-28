#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;
class Plotter;

class TriggerProcessor : public Processor{
  private:
	TriggerStructure structure;
	Wave<int> waveform;

	Plotter *energy_1d;
	Plotter *phase_1d;

	virtual bool HandleEvents();
	
  public:
	TriggerProcessor(MapFile *map_);

	~TriggerProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
