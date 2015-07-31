#include "TriggerProcessor.hpp"

bool TriggerProcessor::HandleEvents(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		total_events++;
	}
	return true;
}

TriggerProcessor::TriggerProcessor(bool write_waveform_/*=false*/) : Processor("Trigger", "trigger"){
	write_waveform = write_waveform_;
}
