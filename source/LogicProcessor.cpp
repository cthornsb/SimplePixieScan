#include "LogicProcessor.hpp"
#include "Structures.h"
#include "MapFile.hpp"

/// Process all individual events.
bool LogicProcessor::HandleEvents(){
	if(!init){ return false; }

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		// Fill the values into the root tree.
		structure.Append((*iter)->entry->location);
		
		good_events++;
	}
	return true;
}

LogicProcessor::LogicProcessor(MapFile *map_) : Processor("Logic", "logic", map_){
	root_structure = (Structure*)&structure;
	use_fitting = false; // No fitting for logic pulses.
}

LogicProcessor::~LogicProcessor(){
}
