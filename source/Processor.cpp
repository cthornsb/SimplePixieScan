#include <iostream>
#include <time.h>

#include "Processor.hpp"
#include "ChannelEvent.hpp"

void Processor::ClearEvents(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		delete (*iter);
	}
	events.clear();
}

bool Processor::HandleEvents(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		total_events++;
	}
	return false;
}

Processor::Processor(std::string name_, std::string type_){
	name = name_;
	type = type_;
	init = false;
	write_waveform = false;
	
	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	total_events = 0;
}

Processor::~Processor(){
	// Ensure there are no events left in the queue
	if(!events.empty()){ ClearEvents(); }
}

float Processor::Status(){
	float time_taken = 0.0;
	//if(init){
		// output the time usage and the number of valid events
		time_taken = ((float)total_time)/CLOCKS_PER_SEC;
		std::cout << " " << name << "Processor: Used " << time_taken << " seconds of CPU time\n";
		std::cout << " " << name << "Processor: Found " << total_events << " total events\n";
		if(total_events > 0){ std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/total_events << "%)\n"; }
	//}
	return time_taken;
}

bool Processor::Process(ChannelEvent *start_){
	// Handle the processor events
	start = start_;
	bool retval = HandleEvents();
	
	// Clean up all channel events which we've been given
	ClearEvents();
	
	return retval;
}
