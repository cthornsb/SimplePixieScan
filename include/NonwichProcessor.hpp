#ifndef NONWICH_PROCESSOR_HPP
#define NONWICH_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class NonwichProcessor : public Processor{
  private:
	NonwichStructure structure;
	NonwichWaveform waveform;  
  
	bool HandleEvents();
	
  public:
	NonwichProcessor(MapFile *map_);

	~NonwichProcessor();
};

#endif
