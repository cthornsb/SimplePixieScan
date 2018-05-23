#ifndef LOGIC_PROCESSOR_HPP
#define LOGIC_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class LogicProcessor : public Processor{
  private:
	LogicStructure structure;
  
	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	LogicProcessor(MapFile *map_);
	
	~LogicProcessor();
};

#endif
