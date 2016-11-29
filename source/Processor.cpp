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

#define ADC_TIME_STEP 4

Structure dummyStructure;
Trace dummyTrace;

const double pi = 3.1415926540;
const double twoPi = 6.283185308;

unsigned short traceX[1250]; // Array of values to use for the x-axis of the trace (up to 5 us long).

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

/**The Paulauskas function is described in NIM A 737 (22), with a slight 
 * adaptation. We use a step function such that f(x < phase) = baseline.
 * In addition, we also we formulate gamma such that the gamma in the paper is
 * gamma_prime = 1 / pow(gamma, 0.25).
 *
 * The parameters are:
 * p[0] = baseline
 * p[1] = amplitude
 * p[2] = phase
 * p[3] = beta
 * p[4] = gamma
 *
 * \param[in] x X value.
 * \param[in] p Paramater values.
 *
 * \return the value of the function for the specified x value and parameters.
 */
double FittingFunction::operator () (double *x, double *p) {
	float diff = (x[0] - p[2])/ADC_TIME_STEP;
	if (diff < 0 ) return p[0];
	return p[0] + p[1] * std::exp(-diff * p[3]) * (1 - std::exp(-std::pow(diff * p[4],4)));
}

FittingFunction::FittingFunction(double beta_/*=0.563362*/, double gamma_/*=0.3049452*/){
	beta = beta_;
	gamma = gamma_;
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
	
	fitting_func = new TF1((type + "_func").c_str(), *actual_func, 0, 1, 5);
	use_fitting = true;
	
	return fitting_func;
}

bool Processor::HandleSingleEndedEvents(){
	if(!init){ return false; }

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		// Check that the time and energy values are valid
		if(!(*iter)->channelEvent->valid_chan){ continue; }
		
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
	if(!init || events.size() <= 1){ 
		return false;
	}
	
	// Sort the vandle event list by channel ID. This way, we will be able
	// to determine which channels are neighbors and, thus, part of the
	// same vandle bar.
	sort(events.begin(), events.end(), &ChannelEventPair::CompareChannel);
	
	std::deque<ChannelEventPair*>::iterator iter_L = events.begin();
	std::deque<ChannelEventPair*>::iterator iter_R = events.begin()+1;

	ChanEvent *current_event_L;
	ChanEvent *current_event_R;

	// Pick out pairs of channels representing vandle bars.
	for(; iter_R != events.end(); iter_L++, iter_R++){
		current_event_L = (*iter_L)->channelEvent;
		current_event_R = (*iter_R)->channelEvent;

		// Check that the time and energy values are valid
		if(!current_event_L->valid_chan || !current_event_R->valid_chan){ continue; }
	
		// Check that these two channels have the correct detector tag.
		if((*iter_L)->entry->subtype != "left" || 
		   (*iter_R)->entry->subtype != "right"){ continue; }
	
		// Check that these two channels are indeed neighbors. If not, iterate up by one and check again.
		if((current_event_L->modNum != current_event_R->modNum) || (current_event_L->chanNum+1 != current_event_R->chanNum)){ continue; }
		
		// Process the individual event.
		if(HandleEvent(*iter_L, *iter_R))
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
	float beta, gamma;
	if(entry_->getArg(0, beta)){ actual_func->SetBeta(beta); }
	if(entry_->getArg(1, gamma)){ actual_func->SetGamma(gamma); }

	// Set initial parameters to those obtained from fit optimizations.
	fitting_func->SetParameter(0, event_->baseline); // Baseline of pulse
	fitting_func->SetParameter(1, 1.5 * event_->qdc); // Normalization of pulse
	fitting_func->SetParameter(2, (event_->max_index-fitting_low)*ADC_TIME_STEP); // Phase (leading edge of pulse) (ns)
	fitting_func->SetParameter(3, beta);
	fitting_func->SetParameter(4, gamma);
	
	// Set the fitting range.
	fitting_func->SetRange((event_->max_index-fitting_low)*ADC_TIME_STEP, (event_->max_index+fitting_high)*ADC_TIME_STEP);
	
	return true;
}

bool Processor::FitPulse(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }
	
	// Set the default fitting function.
	if(!fitting_func){ SetFitFunction(); } 

	// Set the initial fitting parameters.
	if(!SetFitParameters(event_, entry_))
		return false;

	// "Convert" the trace into a TGraph for fitting.
	TGraph *graph = new TGraph(event_->traceLength);
	for(size_t graphIndex = 0; graphIndex < event_->traceLength; graphIndex++)
		graph->SetPoint(graphIndex, traceX[graphIndex], event_->adcTrace[graphIndex]);

	// And finally, do the fitting.
	graph->Fit(fitting_func, "Q R");
	
	// Update the trace parameters.
	event_->baseline = fitting_func->GetParameter(0);
	event_->phase = fitting_func->GetParameter(2)/4.0;
	
	delete graph;
	
	return true;
}

/// Perform CFD analysis on a single trace.
bool Processor::CfdPulse(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }

	// Set the initial CFD parameters.
	if(!SetCfdParameters(event_, entry_))
		return false;

	// Set the CFD threshold point of the trace.
	float cfdF = 0.5;
	float cfdD = 1;
	float cfdL = 1;
	entry_->getArg(0, cfdF);
	entry_->getArg(1, cfdD);
	entry_->getArg(2, cfdL);
	
	// Analyze the trace.
	event_->AnalyzeCFD(cfdF, (int)cfdD, (int)cfdL);
	
	return (event_->phase > 0);
}

Processor::Processor(std::string name_, std::string type_, MapFile *map_){
	name = name_;
	type = type_;
	init = false;
	write_waveform = false;
	use_color_terminal = true;
	use_trace = true;
	use_fitting = false;
	use_integration = true;
	isSingleEnded = true;
	presortData = false;
	
	total_time = 0;
	start_time = clock();
	
	good_events = 0;
	total_events = 0;
	
	root_structure = &dummyStructure;
	root_waveform = &dummyTrace;
	root_waveformR = &dummyTrace;
	
	local_branch = NULL;
	trace_branch = NULL;
	fitting_func = NULL;
	actual_func = NULL;

	fitting_low = 5;
	fitting_high = 10;

	mapfile = map_;
	
	clockInSeconds = 8e-9;
	adcClockInSeconds = 4e-9;
	filterClockInSeconds = 8e-9;
	
	for(int i = 0; i < 1250; i++)
		traceX[i] = i * ADC_TIME_STEP;
}

Processor::~Processor(){
	// Ensure there are no events left in the queue
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
		std::cout << " " << name << "Processor: " << total_events << " Total Events (" << 100.0*total_events/global_events_ << "%)\n";
		if(init) std::cout << " " << name << "Processor: " << good_events << " Valid Events (" << 100.0*good_events/global_events_ << "%)\n";
	}
	
	return time_taken;
}

void Processor::PreProcess(){
	// Start the timer.
	StartProcess(); 
	
	ChanEvent *current_event;

	// Iterate over the list of channel events.
	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		total_events++;
		
		current_event = (*iter)->channelEvent;
		
		if(!presortData){
			// Set the default values for high resolution energy and time.
			current_event->hiresTime = current_event->time * filterClockInSeconds;
		
			// Check for trace with zero size.
			if(current_event->traceLength == 0){
				if(use_trace && !presortData){
					// The trace is required by this processor, but does not exist.
					continue; 
				}				
				// The trace is not required by the processor. Set the channel event to valid.
				current_event->valid_chan = true;
			}
			else{ // The trace exists.
				// Calculate the baseline.
				if(current_event->ComputeBaseline() < 0){ continue; }
		
				// Check for large SNR.
				//if(current_event->stddev > 3.0){ continue; }

				if(use_integration) // Compute the integral of the pulse within the integration window.
					current_event->IntegratePulse(current_event->max_index - fitting_low, current_event->max_index + fitting_high);
		
				// Set the channel event to valid.
				current_event->valid_chan = true;
		
				if(use_fitting){ // Do root fitting for high resolution timing (very slow).
					if(!FitPulse(current_event, (*iter)->entry)){
						// Set the channel event to invalid.
						current_event->valid_chan = false;
						continue;
					}
				}
				else{ // Do a more simplified CFD analysis to save time.
					if(!CfdPulse(current_event, (*iter)->entry)){
						// Set the channel event to invalid.
						current_event->valid_chan = false;
						continue;
					}
				}
			
				// Add the phase of the trace to the high resolution time.
				current_event->hiresTime += current_event->phase * adcClockInSeconds;
			}
		}
		else{
			// This event is from a presorted file. We assume that the event
			// has already passed a screening and is valid to use.
			current_event->valid_chan = true;
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

// Return a random number between low and high.
double drand(const double &low_, const double &high_){
	return low_+(double(rand())/RAND_MAX)*(high_-low_);
}

// Return a randum number between 0 and 1.
double drand(){
	return double(rand())/RAND_MAX;
}

// Add angle1 and angle2 and wrap the result between 0 and 2*pi.
double addAngles(const double &angle1_, const double &angle2_){
	double output = angle1_ + angle2_;
	if(output < 0.0) output += twoPi;
	else if(output > twoPi) output = output - twoPi;
	return output;
}
