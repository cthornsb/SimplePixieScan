#ifndef GENERIC_PROCESSOR_HPP
#define GENERIC_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class GenericProcessor : public Processor{
  private:
	GenericStructure structure;
	GenericWaveform waveform;
  
	bool HandleEvents();
	
  public:
	GenericProcessor(bool write_waveform_=false, bool hires_timing_=false);

	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
