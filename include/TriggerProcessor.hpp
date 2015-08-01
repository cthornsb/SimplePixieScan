#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class TriggerProcessor : public Processor{
  private:
	bool HandleEvents();
	
	TriggerStructure structure;
	TriggerWaveform waveform;

  public:
	TriggerProcessor(bool write_waveform_=false);
	
	bool Initialize(TTree *tree_);
	
	void Zero();
};

#endif
