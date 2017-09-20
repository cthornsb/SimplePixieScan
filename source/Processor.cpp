#include <iostream>
#include <cmath>
#include <time.h>
#include <algorithm>

#include "Processor.hpp"
#include "Structures.h"
#include "MapFile.hpp"
#include "CalibFile.hpp"

#include "TTree.h"
#include "TGraph.h"

Structure dummyStructure;
Trace dummyTrace;

const double pi = 3.1415926540;
const double twoPi = 6.283185308;

ChannelEventPair::ChannelEventPair(){
	channelEvent = NULL;
	calib = NULL;
	entry = NULL;
}

ChannelEventPair::ChannelEventPair(ChanEvent *chan_event_, MapEntry *entry_, CalibEntry *calib_){
	channelEvent = chan_event_;
	calib = calib_;
	entry = entry_;
}

ChannelEventPair::~ChannelEventPair(){
	if(channelEvent){ delete channelEvent; } // Deleting the ChanEvent will also delete the underlying XiaData.
}

// Return a random number between low and high.
double Processor::drand(const double &low_, const double &high_){
	return low_+(double(rand())/RAND_MAX)*(high_-low_);
}

// Return a randum number between 0 and 1.
double Processor::drand(){
	return double(rand())/RAND_MAX;
}

// Add angle1 and angle2 and wrap the result between 0 and 2*pi.
double Processor::addAngles(const double &angle1_, const double &angle2_){
	double output = angle1_ + angle2_;
	if(output < 0.0) output += twoPi;
	else if(output > twoPi) output = output - twoPi;
	return output;
}

void Processor::PrintMsg(const std::string &msg_){
	std::cout << name << "Processor: " << msg_ << std::endl; 
}

bool Processor::HandleSingleEndedEvents(){
	if(!init){ return false; }

	total_handled += events.size();
	
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		// Check that the time and energy values are valid
		if(!(*iter)->channelEvent->valid_chan){ 
			handle_notValid++;
			continue; 
		}
		
		// Process the individual event.
		if(HandleEvent(*iter))
			good_events++;
			
		// Copy the trace to the output file.
		if(write_waveform)
			root_waveform->Append((*iter)->channelEvent->adcTrace, (*iter)->channelEvent->traceLength);
	}

	return true;
}

bool Processor::HandleDoubleEndedEvents(){
	if(!init) return false;

	if(events.size() <= 1){ 
		handle_unpairedEvent++;
		return false;
	}
	
	total_handled += events.size();

	unsigned short idL;
	unsigned short idR;

	ChannelEventPair *pair1, *pair2;
	std::deque<ChannelEventPair*> unpairedEvents=events;
	std::vector<ChannelEventPair*> lefts, rights;

	// Search for pixie channel pairs.	
	while(!unpairedEvents.empty()){
		if(unpairedEvents.size() <= 1){
			handle_unpairedEvent++;
			unpairedEvents.pop_front();
			break;
		}

		pair1 = unpairedEvents.front();
		idL = pair1->channelEvent->getID();

		// Find this event's neighbor.
		bool foundPair = false;
		for(std::deque<ChannelEventPair*>::iterator iter = unpairedEvents.begin()+1; iter != unpairedEvents.end(); ++iter){
			pair2 = (*iter);
			idR = pair2->channelEvent->getID();
			if(idL % 2 == 0){ // Even
				if(idR == idL+1){
					lefts.push_back(pair1);
					rights.push_back(pair2);
					unpairedEvents.erase(iter);
					foundPair = true;
					break;
				}
			}
			else{ // Odd
				if(idL == idR+1){
					lefts.push_back(pair2);
					rights.push_back(pair1);
					unpairedEvents.erase(iter);
					foundPair = true;
					break;
				}
			}
		}

		// Check for unpaired events.
		if(!foundPair) handle_unpairedEvent++;

		unpairedEvents.pop_front();
	}

	ChanEvent *current_event_L;
	ChanEvent *current_event_R;

	std::vector<ChannelEventPair*>::iterator iterL = lefts.begin();
	std::vector<ChannelEventPair*>::iterator iterR = rights.begin();

	// Pick out pairs of channels representing bars.
	for(; iterL != lefts.end() && iterR != rights.end(); ++iterL, ++iterR){
		current_event_L = (*iterL)->channelEvent;
		current_event_R = (*iterR)->channelEvent;

		// Check that the time and energy values are valid
		if(!current_event_L->valid_chan || !current_event_R->valid_chan){ 
			handle_notValid += 2;
			continue; 
		}
	
		// Process the individual event.
		if(HandleEvent((*iterL), (*iterR)))
			good_events += 2;

		// Copy the trace to the output file.
		if(write_waveform){
			root_waveform->Append(current_event_L->adcTrace, current_event_L->traceLength);
			root_waveformR->Append(current_event_R->adcTrace, current_event_R->traceLength);
		}
	}
	
	return true;
}

bool Processor::SetFitParameters(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }
	
	// Set the fixed fitting parameters for a given detector.
	double beta = defaultBeta;
	double gamma = defaultGamma;
	entry_->getArg(0, beta);
	entry_->getArg(1, gamma);
	
	// Update the fitter with the beta and gamma.
	fitter.SetBetaGamma(beta, gamma);
	
	// Set the fitting range.
	fitter.SetFitRange(fitting_low, fitting_high);

	return true;
}

bool Processor::FitPulse(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }
	
	// Set the initial fitting parameters.
	if(!SetFitParameters(event_, entry_))
		return false;

	// Fit the trace.
	fitter.FitPulse(event_);
	
	return true;
}

/// Perform CFD analysis on a single trace.
bool Processor::CfdPulse(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }

	// Set the initial CFD parameters.
	if(!SetCfdParameters(event_, entry_))
		return false;

	// Set the CFD threshold point of the trace.
	double cfdF = defaultCFD[0];
	double cfdD = defaultCFD[1];
	double cfdL = defaultCFD[2];
	entry_->getArg(0, cfdF);
	
	if(analyzer == POLY){ // Polynomial CFD.
		event_->AnalyzePolyCFD(cfdF);
	}
	else if(analyzer == CFD){ // Traditional CFD.
		entry_->getArg(1, cfdD);
		entry_->getArg(2, cfdL);
		event_->AnalyzeCFD(cfdF, (int)cfdD, (int)cfdL);
	}

	return (event_->phase > 0);
}

Processor::Processor(std::string name_, std::string type_, MapFile *map_){
	name = name_;
	type = type_;
	init = false;
	write_waveform = false;
	use_color_terminal = true;
	use_trace = true;
	use_integration = true;
	isSingleEnded = true;
	histsEnabled = false;
	presortData = false;

	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	total_events = 0;
	total_handled = 0;

	handle_notValid = 0;
	handle_unpairedEvent = 0;
	preprocess_emptyTrace = 0;
	preprocess_badBaseline = 0;
	preprocess_badFit = 0;
	preprocess_badCfd = 0;
	
	root_structure = &dummyStructure;
	root_waveform = &dummyTrace;
	root_waveformR = &dummyTrace;
	
	local_branch = NULL;
	trace_branch = NULL;

	fitting_low = 5;
	fitting_high = 10;

	fitting_low2 = -9999;
	fitting_high2 = -9999;

	defaultBeta = 0.5;
	defaultGamma = 0.1;

	mapfile = map_;
	
	clockInSeconds = 8e-9;
	adcClockInSeconds = 4e-9;
	filterClockInSeconds = 8e-9;

	defaultCFD[0] = 0.5;
	defaultCFD[1] = 1;
	defaultCFD[2] = 1;
}

Processor::~Processor(){
}

bool Processor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(type.c_str(), root_structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((type+"_trace").c_str(), root_waveform);
	}
	
	return (init = true);
}

bool Processor::InitializeTraces(TTree *tree_){
	if(trace_branch || !tree_){ 
		PrintMsg("Trace output is already initialized!");
		return false; 
	}
	else if(!write_waveform){
		PrintMsg("Writing of ADC traces is disabled!");
		return false;
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to ADC trace TTree.");
	trace_branch = tree_->Branch(type.c_str(), root_waveform);
	
	return (init = true);
}

float Processor::Status(unsigned long global_events_){
	float time_taken = 0.0;
	
	// output the time usage and the number of valid events
	time_taken = ((float)total_time)/CLOCKS_PER_SEC;
	std::cout << " " << name << "Processor: Used " << time_taken << " seconds of CPU time\n";
	if(total_events > 0){
		std::cout << " " << name << "Processor: " << total_events << " Total Events\n";
		std::cout << " " << name << "Processor: " << total_handled << " Handled Events (" << 100.0*total_handled/total_events << "%)\n";
		if(init) std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/total_events << "%)\n";
	}

	// Error codes.
	if(handle_notValid > 0){	
		std::cout << "  [1] Not Valid:      " << handle_notValid << std::endl;
		if(preprocess_emptyTrace > 0)  std::cout << "   [a] Empty Trace:   " << preprocess_emptyTrace << std::endl;
		if(preprocess_badBaseline > 0) std::cout << "   [b] Bad Baseline:  " << preprocess_badBaseline << std::endl;
		if(preprocess_badFit > 0)      std::cout << "   [c] Fit Failure:   " << preprocess_badFit << std::endl;
		if(preprocess_badCfd > 0)      std::cout << "   [d] CFD Failure:   " << preprocess_badCfd << std::endl;
	}
	if(handle_unpairedEvent > 0)                 std::cout << "  [2] Unpaired Event: " << handle_unpairedEvent << std::endl;
	if(handle_notValid+handle_unpairedEvent > 0) std::cout << "  [3] Total Invalid:  " << handle_notValid+handle_unpairedEvent << std::endl;

	return time_taken;
}

void Processor::PreProcess(){
	if(events.empty()) return;

	// Start the timer.
	StartProcess(); 
	
	ChanEvent *current_event;

	total_events += events.size();

	// Iterate over the list of channel events.
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->channelEvent;
		
		if(!presortData){
			// Set the default values for high resolution energy and time.
			current_event->hiresTime = current_event->time * filterClockInSeconds;
		
			// Check for trace with zero size.
			if(current_event->traceLength == 0){
				if(use_trace && !presortData){
					// The trace is required by this processor, but does not exist.
					preprocess_emptyTrace++;
					continue; 
				}				
				// The trace is not required by the processor. Set the channel event to valid.
				current_event->valid_chan = true;
			}
			else{ // The trace exists.
				// Calculate the baseline.
				if(current_event->ComputeBaseline() < 0){
					preprocess_badBaseline++;
					continue; 
				}
		
				// Check for large SNR.
				//if(current_event->stddev > 3.0){ continue; }

				// Compute the integral of the pulse within the integration window.
				current_event->IntegratePulse(current_event->max_index - fitting_low, current_event->max_index + fitting_high);
				if(fitting_low2 != -9999 && fitting_high2 != -9999) 
					current_event->IntegratePulse(current_event->max_index - fitting_low2, current_event->max_index + fitting_high2, true);		
		
				// Set the channel event to valid.
				current_event->valid_chan = true;
		
				if(analyzer == FIT){ // Do root fitting for high resolution timing (very slow).
					if(!FitPulse(current_event, (*iter)->entry)){
						// Set the channel event to invalid.
						current_event->valid_chan = false;
						preprocess_badFit++;
						continue;
					}
				}
				else{ // Do a more simplified CFD analysis to save time.
					if(!CfdPulse(current_event, (*iter)->entry)){
						// Set the channel event to invalid.
						current_event->valid_chan = false;
						preprocess_badCfd++;
						continue;
					}
				}
			
				// Add the phase of the trace to the high resolution time.
				current_event->hiresTime += current_event->phase * adcClockInSeconds;
			}
		}
		
		// Calibrate the energy, if applicable.
		if((*iter)->calib->Energy()){
			if(use_integration)
				current_event->qdc = (*iter)->calib->energyCal->GetCalEnergy(current_event->qdc);
			else
				current_event->energy = (*iter)->calib->energyCal->GetCalEnergy(current_event->energy);
		}
	}

	// Stop the timer.
	StopProcess();
}

bool Processor::Process(ChannelEventPair *start_){
	if(events.empty()) return false;

	// Start the timer.
	StartProcess(); 

	// Set the start event.
	start = start_;

	// Process the individual events.
	bool retval = false;
	if(isSingleEnded)
		retval = HandleSingleEndedEvents();
	else
		retval = HandleDoubleEndedEvents();
	
	// Stop the timer.
	StopProcess(); 
	
	return retval;
}

/** WrapUp processing of events by clearing the event list. Calls to this method
  * must be done after processing is completed since other processor types may
  * rely upon events which are contained within this processor.
  */
void Processor::WrapUp(){
	// No need to delete anything. Scanner will delete all events.
	// We simply need to clear our deque of events.
	events.clear();
}

void Processor::Zero(){
	root_structure->Zero();
	root_waveform->Zero();
}

void Processor::RemoveByTag(const std::string &tag_, const bool &withTag_/*=true*/){
	std::deque<ChannelEventPair*> tempList;
	while(!events.empty()){
		if((withTag_ && events.front()->entry->hasTag(tag_)) || (!withTag_ && events.front()->entry->hasTag(tag_))) 
			tempList.push_back(events.front());
		events.pop_front();
	}
	for(std::deque<ChannelEventPair*>::iterator iter = tempList.begin(); iter != tempList.end(); ++iter){
		events.push_back(*iter);
	}
	tempList.clear();
}
