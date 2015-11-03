#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class TriggerProcessor : public Processor{
  private:
	TriggerStructure structure;
	TriggerWaveform waveform;

	bool HandleEvents();
	
  public:
	TriggerProcessor(MapFile *map_);

	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
