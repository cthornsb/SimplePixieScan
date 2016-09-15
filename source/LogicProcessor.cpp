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

	// Do not force the use of a trace. By setting this flag to false,
	// this processor WILL NOT reject events which do not have an ADC trace.
	use_trace = false;
}

LogicProcessor::~LogicProcessor(){
}
