#include "ChannelEvent.hpp"

ChannelEvent::ChannelEvent(){
	energy = 0.0; 
	time = 0.0;
	
	for(int i = 0; i < numQdcs; i++){
		qdcValue[i] = 0;
	}

	modNum = 0;
	chanNum = 0;
	trigTime = 0;
	cfdTime = 0;
	eventTimeLo = 0;
	eventTimeHi = 0;

	virtualChannel = false;
	pileupBit = false;
	saturatedBit = false;
	
	entry = NULL;
}

bool CompareTime(ChannelEvent *a_, ChannelEvent *b_){ 
	return (a_->time < b_->time); 
}
