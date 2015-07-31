#include <iostream>
#include <time.h>

#include "Processor.hpp"
#include "ChannelEvent.hpp"

void Processor::ClearEvents(){
	while(!events.empty()){
		delete events.front();
		events.pop();
	}
}

bool Processor::HandleEvents(){
	return false;
}

Processor::Processor(std::string name_/*="Generic"*/){
	name = name_;
	init = false;
	
	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	total_events = 0;
}

Processor::~Processor(){
}

float Processor::Status(unsigned int total_events){
	float time_taken = 0.0;
	if(init){
		// output the time usage and the number of valid events
		time_taken = ((float)total_time)/CLOCKS_PER_SEC;
		std::cout << " " << name << "Processor: Used " << time_taken << " seconds of CPU time\n";
		if(total_events > 0){ std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/total_events << "%)\n"; }
	}
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
