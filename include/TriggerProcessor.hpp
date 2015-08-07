#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class TriggerProcessor : public Processor{
  private:
	TriggerStructure structure;
	TriggerWaveform waveform;

	bool HandleEvents();
	
  public:
	TriggerProcessor(bool write_waveform_=false, bool hires_timing_=false);

	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
