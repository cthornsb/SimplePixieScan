#include <cmath>

#include "GenericBarProcessor.hpp"
#include "CalibFile.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool GenericBarProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *channel_event_L = chEvt->channelEvent;
	ChanEvent *channel_event_R = chEvtR->channelEvent;
	
	// Calculate the time difference between the current event and the start.
	double tdiff_L = (channel_event_L->time - start->channelEvent->time)*8 + (channel_event_L->phase - start->channelEvent->phase)*4;
	double tdiff_R = (channel_event_R->time - start->channelEvent->time)*8 + (channel_event_R->phase - start->channelEvent->phase)*4;

	// Get the location of this detector.
	int location = chEvt->entry->location;

	// Fill the values into the root tree.
	structure.Append(tdiff_L, tdiff_R, channel_event_L->qdc, channel_event_R->qdc, location);

	return true;
}

GenericBarProcessor::GenericBarProcessor(MapFile *map_) : Processor("GenericBar", "genericbar", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &L_waveform;
	
	// Set the detector type to a bar.
	isSingleEnded = false;

	// Turn off diagnostic histograms.
	histsEnabled = false;
}

GenericBarProcessor::~GenericBarProcessor(){ 
}

void GenericBarProcessor::GetHists(std::vector<Plotter*> &plots_){
}
