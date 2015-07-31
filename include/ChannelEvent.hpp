#ifndef CHANNELEVENT_HPP
#define CHANNELEVENT_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>

struct MapEntry;

struct ChannelEvent{
	double energy; /// Raw pixie energy
	double time; /// Raw low-res pixie time
	std::vector<unsigned int> trace; /// Trace capture
	
	static const int numQdcs = 8;	 /// Number of QDCs onboard
	unsigned int qdcValue[numQdcs];  /// QDCs from onboard

	int modNum; /// Module number
	int chanNum; /// Channel number
	unsigned int trigTime; /// The channel trigger time, trigger time and the lower 32 bits of the event time are not necessarily the same but could be separated by a constant value.
	unsigned int cfdTime; /// CFD trigger time in units of 1/256 pixie clock ticks
	unsigned int eventTimeLo; /// Lower 32 bits of pixie16 event time
	unsigned int eventTimeHi; /// Upper 32 bits of pixie16 event time

	bool virtualChannel; /// Flagged if generated virtually in Pixie DSP
	bool pileupBit; /// Pile-up flag from Pixie
	bool saturatedBit; /// Saturation flag from Pixie
	
	MapEntry *entry; /// Pointer to the map entry containing detector information
	
	ChannelEvent();
	
	int GetID(){ return modNum*chanNum; }
	
	static bool CompareTime(ChannelEvent *lhs, ChannelEvent *rhs){ return (lhs->time < rhs->time); }
};

#endif
