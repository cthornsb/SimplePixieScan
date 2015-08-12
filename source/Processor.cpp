#include <iostream>
#include <time.h>

#include "Processor.hpp"
#include "ChannelEvent.hpp"

#include "TTree.h"
#include "TF1.h"
#include "TGraph.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"
#include "MapFile.hpp"

// 1D function to use for pulse fitting
// x[0] = time t in ns
// parameters: 4
//  par[0] = alpha (normalization of pulse 1)
//  par[1] = phi (phase of pulse 1 in ns)
//  par[2] = beta (decay parameter of the 1st pulse exponential in ns)
//  par[3] = gamma (width of the inverted square gaussian of the 1st pulse in ns^4)
double func(double *x, double *par){
	double arg = x[0] - par[1];
	if(arg >= 0.0){ return par[0]*std::exp(-arg/par[2])*(1 - std::exp(-arg*arg*arg*arg/par[3])); }
	return 0.0;
}

void Processor::ClearEvents(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		delete (*iter);
	}
	events.clear();
}

void Processor::PrintMsg(const std::string &msg_){
	std::cout << name << "Processor: " << msg_ << std::endl; 
}

void Processor::PrintError(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;31m" << name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

void Processor::PrintWarning(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;33m" << name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

void Processor::PrintNote(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;34m" << name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << name << "Processor: " << msg_ << std::endl; }
}

void Processor::SetFitFunction(double (*func_)(double *, double *), int npar_){
	if(fitting_func){ delete fitting_func; }
	
	fitting_func = new TF1((type + "_func").c_str(), func_, 0, 1, npar_);
	hires_timing = true;
}

void Processor::SetFitFunction(const char* func_){
	if(fitting_func){ delete fitting_func; }
	
	fitting_func = new TF1((type + "_func").c_str(), func_, 0, 1);
	hires_timing = true;
}

void Processor::SetFitParameters(double *data){
	if(!data){ return; }
	
	// Set initial parameters to those obtained from fit optimizations	
	fitting_func->SetParameter(0, data[0]*9.211 + 150.484);  // Normalization of pulse
	fitting_func->SetParameter(1, data[1]*1.087 - 2.359); // Phase (leading edge of pulse) (ns)
	fitting_func->FixParameter(2, 1.7750575); // Decay constant of exponential (ns)
	fitting_func->FixParameter(3, 115.64125); // Width of inverted square guassian (ns^4)
}

void Processor::FitPulses(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		// Set the default values for high resolution energy and time
		(*iter)->hires_energy = (*iter)->energy;
		(*iter)->hires_time = (*iter)->time * filterClockInSeconds;
	
		// Check for trace with zero size
		if((*iter)->trace.size() == 0){ continue; }

		// Find the leading edge of the pulse. This will also set the phase of the ChannelEvent
		if((*iter)->FindLeadingEdge() < 0){ continue; }

		// Set the high resolution energy
		(*iter)->hires_energy = (*iter)->FindQDC();
		
		// Do root fitting for high resolution timing (very slow)
		if(hires_timing){
			// "Convert" the trace into a TGraph for fitting
			TGraph *graph = (*iter)->GetTrace();
		
			double pars[2] = {(double)(*iter)->maximum, (double)(*iter)->phase};
		
			// Set the initial fitting parameters
			SetFitParameters(pars);
			
			// And finally, do the fitting
			TFitResultPtr fit_ptr = graph->Fit(fitting_func, "S Q", "", (*iter)->phase, (*iter)->max_index);
	
			// Update the phase of the pulse
			(*iter)->phase = fitting_func->GetParameter(1);
		
			delete graph;
		}
		
		// Set the high resolution time
		(*iter)->hires_time += (*iter)->phase * adcClockInSeconds;
		(*iter)->valid_chan = true;
	}
}

bool Processor::HandleEvents(){
	for(std::deque<ChannelEvent*>::iterator iter = events.begin(); iter != events.end(); iter++){
		good_events++;
	}
	return false;
}

Processor::Processor(std::string name_, std::string type_, bool hires_timing_/*=true*/){
	name = name_;
	type = type_;
	init = false;
	write_waveform = false;
	use_color_terminal = true;
	hires_timing = hires_timing_;
	
	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	
	local_branch = NULL;
	if(hires_timing){ // If hi-res timing is to be used, set to the vandle fitting function
		fitting_func = new TF1((type + "_func").c_str(), func, 0, 1, 4); 
	}
	else{ fitting_func = NULL; }
}

Processor::~Processor(){
	// Ensure there are no events left in the queue
	if(!events.empty()){ ClearEvents(); }
	if(fitting_func){ delete fitting_func; }
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

bool Processor::Process(ChannelEvent *start_){
	StartProcess();

	// Handle the processor events
	start = start_;
	
	bool retval = HandleEvents();
	
	// Clean up all channel events which we've been given
	ClearEvents();
	
	StopProcess();
	
	return retval;
}
