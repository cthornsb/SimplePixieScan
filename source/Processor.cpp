#include <iostream>
#include <time.h>

#include "Processor.hpp"

#include "TTree.h"
#include "TF1.h"
#include "TGraph.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "MapFile.hpp"

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

void Processor::SetFitParameters(ChannelEventPair *pair_){
	if(!pair_){ return; }

	// Set initial parameters to those obtained from fit optimizations	
	fitting_func->SetParameter(0, pair_->event->maximum*9.211 + 150.484); // Normalization of pulse
	fitting_func->SetParameter(1, pair_->event->phase*1.087 - 2.359); // Phase (leading edge of pulse) (ns)
}

void Processor::FitPulses(){
	if(use_fitting && !fitting_func){ SetFitFunction(); } // Set the default fitting function.

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
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

		// Set the high resolution energy.
		current_event->hires_energy = current_event->FindQDC();
		
		// Do root fitting for high resolution timing (very slow).
		if(use_fitting){
			// "Convert" the trace into a TGraph for fitting.
			TGraph *graph = new TGraph(current_event->size, current_event->xvals, current_event->yvals);
		
			// Set the initial fitting parameters.
			SetFitParameters((*iter));
			
			// And finally, do the fitting.
			TFitResultPtr fit_ptr = graph->Fit(fitting_func, "S Q", "", current_event->max_index-fitting_low, current_event->max_index+fitting_high);
	
			// Update the phase of the pulse.
			current_event->phase = fitting_func->GetParameter(1);
		
			delete graph;
		}
		
		// Set the high resolution time.
		current_event->hires_time += current_event->phase * adcClockInSeconds;
		current_event->valid_chan = true;
	}
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
	
	local_branch = NULL;
	fitting_func = NULL;
	actual_func = NULL;

	fitting_low = 10;
	fitting_high = 15;

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
	return false;
}

float Processor::Status(unsigned long total_events_){
	float time_taken = 0.0;
	if(init){
		// output the time usage and the number of valid events
		time_taken = ((float)total_time)/CLOCKS_PER_SEC;
		std::cout << " " << name << "Processor: Used " << time_taken << " seconds of CPU time\n";
		if(good_events > 0){ std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/total_events_ << "%)\n"; }
	}
	return time_taken;
}

void Processor::PreProcess(){
	StartProcess();
	FitPulses();
	StopProcess();
}

bool Processor::Process(ChannelEventPair *start_){
	StartProcess();

	// Handle the processor events
	start = start_;
	
	bool retval = HandleEvents();
	
	// Clean up all channel events which we've been given
	ClearEvents();
	
	StopProcess();
	
	return retval;
}
