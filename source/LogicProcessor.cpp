#include "LogicProcessor.hpp"
#include "Structures.h"
#include "MapFile.hpp"

/// Process all individual events.
bool LogicProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	// Fill the values into the root tree.
	structure.Append(chEvt->entry->location);
	
	return true;
}

LogicProcessor::LogicProcessor(MapFile *map_) : Processor("Logic", "logic", map_){
	root_structure = (Structure*)&structure;
	use_fitting = false; // No fitting for logic pulses.
}

LogicProcessor::~LogicProcessor(){
}
