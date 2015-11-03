#ifndef GENERIC_PROCESSOR_HPP
#define GENERIC_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class GenericProcessor : public Processor{
  private:
	GenericStructure structure;
	GenericWaveform waveform;
  
	bool HandleEvents();
	
  public:
	GenericProcessor(MapFile *map_);

	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
