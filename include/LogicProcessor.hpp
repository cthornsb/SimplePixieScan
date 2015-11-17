#ifndef LOGIC_PROCESSOR_HPP
#define LOGIC_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class LogicProcessor : public Processor{
  private:
	LogicStructure structure;
  
	/// Process all individual events.
	virtual bool HandleEvents();
	
  public:
	LogicProcessor(MapFile *map_);
	
	~LogicProcessor();
};

#endif
