#include "TriggerProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

bool TriggerProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;

	if(histsEnabled){
		// Get the location of this detector.
		int location = chEvt->entry->location;

		// Fill all diagnostic histograms.
		energy_1d->Fill(location, current_event->qdc);
		phase_1d->Fill(location, current_event->phase);
	}

	// Fill the values into the root tree.
	structure.Append(current_event->time, current_event->phase, current_event->qdc);
	
	return true;
}

TriggerProcessor::TriggerProcessor(MapFile *map_) : Processor("Trigger", "trigger", map_){
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;

	energy_1d = NULL;
	phase_1d = NULL;
}

void TriggerProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(energy_1d);
	online_->GenerateHist(phase_1d);

	histsEnabled = true;
}
