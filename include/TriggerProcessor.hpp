#ifndef TRIGGER_PROCESSOR_HPP
#define TRIGGER_PROCESSOR_HPP

#include "Processor.hpp"

class TriggerProcessor : public Processor{
  private:
	bool HandleEvents();

  public:
	TriggerProcessor(bool write_waveform_=false);
};

#endif
