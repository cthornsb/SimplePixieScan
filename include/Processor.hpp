#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <deque>

#include "ChannelEvent.hpp"

#include "TF1.h"
#include "TFitResultPtr.h"

class MapEntry;
class MapFile;

class TTree;
class TBranch;
class TGraph;
class TF1;

class Structure;
class Waveform;

extern Structure dummyStructure;
extern Waveform dummyWaveform;

class ChannelEventPair{
  public:
  	ChannelEvent *event;
	MapEntry *entry;
  
	ChannelEventPair();
	
	ChannelEventPair(ChannelEvent *event_, MapEntry *entry_);
	
	~ChannelEventPair();
	
	/// Return true if the time of arrival for rhs is later than that of lhs.
	static bool CompareTime(ChannelEventPair *lhs, ChannelEventPair *rhs){ return (lhs->event->time < rhs->event->time); }
	
	/// Return true if lhs has a lower event id (mod * chan) than rhs.
	static bool CompareChannel(ChannelEventPair *lhs, ChannelEventPair *rhs){ return ((lhs->event->modNum*lhs->event->chanNum) < (rhs->event->modNum*rhs->event->chanNum)); }
};

class FittingFunction{
  protected:
	double beta;
	double gamma;

  public:
  	FittingFunction(double beta_=0.563362, double gamma_=0.3049452);

	virtual ~FittingFunction(){}

  	double GetBeta(){ return beta; }
  	
  	double GetGamma(){ return gamma; }
  	
  	double SetBeta(const double &beta_){ return (beta = beta_); }
  	
  	double SetGamma(const double &gamma_){ return (gamma = gamma_); }
  	
	virtual double operator () (double *x, double *par);
};

class Processor{
  protected:
	Structure *root_structure; /// Root data structure for storing processor-specific information.
	Waveform *root_waveform; /// Root data structure for storing baseline corrected traces.
  
	clock_t start_time;
	unsigned long total_time;
	
	unsigned long good_events;
	unsigned long total_events;
	
	ChannelEventPair *start;
	std::deque<ChannelEventPair*> events;
	
	std::string name;
	std::string type;
	bool init;
	bool write_waveform;
	bool use_color_terminal;
	bool use_fitting;
	
	TBranch *local_branch;
	TF1 *fitting_func;

	FittingFunction *actual_func;

	TFitResultPtr fit_result; /// Fit result pointer for storing trace fitting information.

	int fitting_low;
	int fitting_high;
	
	MapFile *mapfile;

	double clockInSeconds; /// One pixie clock is 8 ns
	double adcClockInSeconds; /// One ADC clock is 4 ns
	double filterClockInSeconds; /// One filter clock is 8 ns

	/// Clear channel events from the queue
	void ClearEvents();
  
	/// Start the process timer
	void StartProcess(){ start_time = clock(); }
	
	/// Update the amount of time taken by the processor
	void StopProcess(){ total_time += (clock() - start_time); }

	void PrintMsg(const std::string &msg_);
	
	void PrintError(const std::string &msg_);
	
	void PrintWarning(const std::string &msg_);
	
	void PrintNote(const std::string &msg_);
	
	TF1 *SetFitFunction(double (*func_)(double *, double *), int npar_);
	
	TF1 *SetFitFunction(const char* func_);
	
	TF1 *SetFitFunction();

	/// Set the fit parameters for the current event.
	virtual bool SetFitParameters(ChannelEvent *event_, MapEntry *entry_);
	
	/// Fit a single trace.
	virtual bool FitPulse(TGraph *trace_, float &phase);	

	/// Process all individual events.
	virtual bool HandleEvents();

  public:
	Processor(std::string name_, std::string type_, MapFile *map_);
	
	virtual ~Processor();
	
	std::string GetType(){ return type; }
	
	std::string GetName(){ return name; }
	
	TF1 *GetFunction(){ return fitting_func; }
	
	bool IsInit(){ return init; }
	
	bool ToggleFitting(){ return (use_fitting = !use_fitting); }
	
	bool Initialize(TTree *tree_);

	float Status(unsigned long global_events_);

	void AddEvent(ChannelEventPair *event_){ events.push_back(event_); }

	void PreProcess();

	bool Process(ChannelEventPair *start_);
	
	/// Finish processing of events by clearing the event list.
	void WrapUp();
	
	void Zero();
};

#endif
