#ifndef VANDLE_PROCESSOR_HPP
#define VANDLE_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class VandleProcessor : public Processor{
  private:
	VandleStructure structure;
	VandleWaveform waveform;  
  
	bool HandleEvents();
	
  public:
	VandleProcessor(MapFile *map_);

	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
