#include <iostream>
#include <cmath>
#include <time.h>

#include "Processor.hpp"
#include "Structures.h"
#include "MapFile.hpp"

#include "TTree.h"
#include "TGraph.h"

Structure dummyStructure;
Waveform dummyWaveform;

ChannelEventPair::ChannelEventPair(){
	event = NULL;
	entry = NULL;
}

ChannelEventPair::ChannelEventPair(ChannelEvent *event_, MapEntry *entry_){
	event = event_;
	entry = entry_;
}

ChannelEventPair::~ChannelEventPair(){
	if(event){ delete event; }
}

// 1D function to use for pulse fitting
// x[0] = time t in ns
// parameters: 4
//  par[0] = alpha (normalization of pulse 1)
//  par[1] = phi (phase of pulse 1 in ns)
//  par[2] = beta (decay parameter of the 1st pulse exponential in ns) (fixed)
//  par[3] = gamma (width of the inverted square gaussian of the 1st pulse in ns^4) (fixed)
double FittingFunction::operator () (double *x, double *par){
	double arg = x[0] - par[1];
	if(arg >= 0.0){ return par[0]*std::exp(-arg*beta)*(1 - std::exp(-std::pow(arg*gamma, 4.0))); }
	return 0.0;
}

FittingFunction::FittingFunction(double beta_/*=0.563362*/, double gamma_/*=0.3049452*/){
	beta = beta_;
	gamma = gamma_;
}

void Processor::ClearEvents(){
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		delete (*iter);
	}
	events.clear();
}

void Processor::PrintMsg(const std::string &msg_){
	std::cout << name << "Processor: " << msg_ << std::endl; 
}

void Processor::PrintError(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << name << "Processor: \033[1;31m" << msg_ << "\033[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

void Processor::PrintWarning(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << name << "Processor: \033[1;33m" << msg_ << "\033[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

void Processor::PrintNote(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << name << "Processor: \033[1;34m" << msg_ << "\033[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

TF1 *Processor::SetFitFunction(double (*func_)(double *, double *), int npar_){
	if(fitting_func){ delete fitting_func; }
	
	fitting_func = new TF1((type + "_func").c_str(), func_, 0, 1, npar_);
	use_fitting = true;
	
	return fitting_func;
}

TF1 *Processor::SetFitFunction(const char* func_){
	if(fitting_func){ delete fitting_func; }
	
	fitting_func = new TF1((type + "_func").c_str(), func_, 0, 1);
	use_fitting = true;
	
	return fitting_func;
}

TF1 *Processor::SetFitFunction(){
	if(!actual_func){ actual_func = new FittingFunction(); }
	if(fitting_func){ delete fitting_func; }
	
	fitting_func = new TF1((type + "_func").c_str(), *actual_func, 0, 1, 2);
	use_fitting = true;
	
	return fitting_func;
}

bool Processor::SetFitParameters(ChannelEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }
	
	// Set the fixed fitting parameters for a given detector.
	actual_func->SetBeta(entry_->beta);
	actual_func->SetGamma(entry_->gamma);

	// Set initial parameters to those obtained from fit optimizations.
	fitting_func->SetParameter(0, event_->maximum*9.211 + 150.484); // Normalization of pulse
	fitting_func->SetParameter(1, event_->phase*1.087 - 2.359); // Phase (leading edge of pulse) (ns)
	
	// Set the fitting range.
	fitting_func->SetRange(event_->max_index-fitting_low, event_->max_index+fitting_high);
	
	return true;
}

bool Processor::FitPulse(TGraph *trace_, float &phase){
	if(!trace_){ return false; }
	
	// Set the default fitting function.
	if(!fitting_func){ SetFitFunction(); } 

	// And finally, do the fitting.
	fit_result = trace_->Fit(fitting_func, "S Q R");
	
	// Update the phase of the pulse.
	phase = fitting_func->GetParameter(1);
	
	return true;
}

bool Processor::HandleEvents(){
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		good_events++;
	}
	return false;
}

Processor::Processor(std::string name_, std::string type_, MapFile *map_){
	name = name_;
	type = type_;
	init = false;
	write_waveform = false;
	use_color_terminal = true;
	use_fitting = true;
	
	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	total_events = 0;
	
	root_structure = &dummyStructure;
	root_waveform = &dummyWaveform;
	
	local_branch = NULL;
	fitting_func = NULL;
	actual_func = NULL;

	fitting_low = 5;
	fitting_high = 10;

	mapfile = map_;
	
	clockInSeconds = 8e-9;
	adcClockInSeconds = 4e-9;
	filterClockInSeconds = 8e-9;
}

Processor::~Processor(){
	// Ensure there are no events left in the queue
	if(!events.empty()){ ClearEvents(); }
	if(fitting_func){ delete fitting_func; }
	if(actual_func){ delete actual_func; }
}

bool Processor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(name.c_str(), root_structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((name+"Wave").c_str(), root_waveform);
	}
	
	return (init = true);
}

float Processor::Status(unsigned long global_events_){
	float time_taken = 0.0;
	if(init){
		// output the time usage and the number of valid events
		time_taken = ((float)total_time)/CLOCKS_PER_SEC;
		std::cout << " " << name << "Processor: Used " << time_taken << " seconds of CPU time\n";
		if(total_events > 0){
			std::cout << " " << name << "Processor: " << total_events << " Total Events (" << 100.0*total_events/global_events_ << "%)\n";
			std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/total_events << "%)\n";
		}
	}
	return time_taken;
}

void Processor::PreProcess(){
	// Start the timer.
	StartProcess(); 
	
	ChannelEvent *current_event;

	// Iterate over the list of channel events.
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		total_events++;
		
		current_event = (*iter)->event;
	
		// Set the default values for high resolution energy and time.
		current_event->hires_energy = current_event->energy;
		current_event->hires_time = current_event->time * filterClockInSeconds;
	
		// Check for trace with zero size.
		if(current_event->trace.size() == 0){ continue; }

		// Correct the baseline.
		if(current_event->CorrectBaseline() < 0){ continue; }
		
		// Check for large SNR.
		if(current_event->stddev > 3.0){ continue; }

		// Find the leading edge of the pulse. This will also set the phase of the ChannelEventPair.
		if(current_event->FindLeadingEdge() < 0){ continue; }

		// Compute the energy of the pulse within the fitting window.
		current_event->hires_energy = current_event->FindQDC(current_event->max_index - fitting_low, current_event->max_index + fitting_high);
		
		// Set the channel event to valid.
		current_event->valid_chan = true;
		
		// Do root fitting for high resolution timing (very slow).
		if(use_fitting){
			if(!fitting_func){ SetFitFunction(); }
		
			// "Convert" the trace into a TGraph for fitting.
			TGraph *graph = new TGraph(current_event->size, current_event->xvals, current_event->yvals);
		
			// Set the initial fitting parameters.
			if(SetFitParameters(current_event, (*iter)->entry)){
				// Fit the TGraph.
				if(!FitPulse(graph, current_event->phase)){
					// Set the channel event to invalid.
					current_event->valid_chan = false;
				}
			}
			
			delete graph;
		}
		
		// Set the high resolution time.
		current_event->hires_time += current_event->phase * adcClockInSeconds;
	}

	// Stop the timer.
	StopProcess();
}

bool Processor::Process(ChannelEventPair *start_){
	// Start the timer.
	StartProcess(); 

	// Set the start event.
	start = start_;
	
	// Process the individual events.
	bool retval = HandleEvents();
	
	// Stop the timer.
	StopProcess(); 
	
	return retval;
}

/** WrapUp processing of events by clearing the event list. Calls to this method
  * must be done after processing is completed since other processor types may
  * rely upon events which are contained within this processor.
  */
void Processor::WrapUp(){
	// Clean up all channel events which we've been given
	ClearEvents();
}

void Processor::Zero(){
	root_structure->Zero();
	root_waveform->Zero();
}
