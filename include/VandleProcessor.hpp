#ifndef VANDLE_PROCESSOR_HPP
#define VANDLE_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class VandleProcessor : public Processor{
  private:
	bool HandleEvents();
	
	VandleStructure structure;
	VandleWaveform waveform;

  public:
	VandleProcessor(bool write_waveform_=false);
	
	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
