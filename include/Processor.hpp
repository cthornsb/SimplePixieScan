#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <deque>

#include "XiaData.hpp"
#include "TraceFitter.hpp"

#include "TF1.h"
#include "TFitResultPtr.h"

#include "Structures.hpp"

class MapEntry;
class MapFile;
class Plotter;

class TTree;
class TBranch;
class TH1;

extern Structure dummyStructure;
extern Trace dummyTrace;

extern const double pi;
extern const double twoPi;

typedef ChannelEvent ChanEvent;

enum TimingAnalyzer { POLY=0, CFD=1, FIT=2 };

class ChannelEventPair{
  public:
  	ChanEvent *channelEvent;
	MapEntry *entry;
  
	ChannelEventPair();
	
	ChannelEventPair(ChanEvent *c_event_, MapEntry *entry_);
	
	~ChannelEventPair();
	
	/// Return true if the time of arrival for rhs is later than that of lhs.
	static bool CompareTime(ChannelEventPair *lhs, ChannelEventPair *rhs){ return (lhs->channelEvent->time < rhs->channelEvent->time); }
	
	/// Return true if lhs has a lower event id (mod * chan) than rhs.
	static bool CompareChannel(ChannelEventPair *lhs, ChannelEventPair *rhs){ return ((lhs->channelEvent->modNum*16+lhs->channelEvent->chanNum) < (rhs->channelEvent->modNum*16+rhs->channelEvent->chanNum)); }
};

class Processor{
  private:
	std::deque<ChannelEventPair*> events;	  

	std::string name;
	std::string type;
	bool init;
	bool use_color_terminal;

	clock_t start_time;
	unsigned long total_time;
	
	unsigned long good_events;
	unsigned long total_events;
	unsigned long total_handled;

	unsigned long handle_notValid;
	unsigned long handle_unpairedEvent;
	unsigned long preprocess_emptyTrace;
	unsigned long preprocess_badBaseline;
	unsigned long preprocess_badFit;
	unsigned long preprocess_badCfd;

	TBranch *local_branch;
	TBranch *trace_branch;

	double clockInSeconds; /// One pixie clock is 8 ns
	double adcClockInSeconds; /// One ADC clock is 4 ns
	double filterClockInSeconds; /// One filter clock is 8 ns

	TimingAnalyzer analyzer; /// Specifies which type of high-resolution timing is to be used.

	TraceFitter fitter; /// High-resolution fitter used for fitting traces.

  protected:
	Structure *root_structure; /// Root data structure for storing processor-specific information.
	Trace *root_waveform; /// Root data structure for storing traces.
	Trace *root_waveformR; /// Root data structure for storing right detector traces.

	float defaultCFD[3]; /// Default CFD parameters (F, D, L)

	bool use_trace; /// Force the use of the ADC trace. Any events without a trace will be rejected.
	bool write_waveform;
	bool isSingleEnded;
	bool histsEnabled;

	int fitting_low, fitting_low2;
	int fitting_high, fitting_high2;

	double defaultBeta; /// Default beta value to use for fitting.
	double defaultGamma; /// Default gamma value to use for fitting.

	ChannelEventPair *start;

	MapFile *mapfile;

	// Return a random number between low and high.
	double drand(const double &low_, const double &high_);

	// Return a randum number between 0 and 1.
	double drand();

	// Add angle1 and angle2 and wrap the result between 0 and 2*pi.
	double addAngles(const double &angle1_, const double &angle2_);
  
	/// Start the process timer
	void StartProcess(){ start_time = clock(); }
	
	/// Update the amount of time taken by the processor
	void StopProcess(){ total_time += (clock() - start_time); }

	void PrintMsg(const std::string &msg_);
	
	TF1 *SetFitFunction(double (*func_)(double *, double *), int npar_);
	
	TF1 *SetFitFunction(const char* func_);
	
	TF1 *SetFitFunction();

	bool HandleSingleEndedEvents();
	
	bool HandleDoubleEndedEvents();

	/// Set the fit parameters for the current event.
	virtual bool SetFitParameters(ChanEvent *event_, MapEntry *entry_);
	
	/// Fit a single trace.
	virtual bool FitPulse(ChanEvent *event_, MapEntry *entry_);	

	/// Set the CFD parameters for the current event.
	virtual bool SetCfdParameters(ChanEvent *event_, MapEntry *entry_){ return true; }

	/// Perform CFD analysis on a single trace.
	virtual bool CfdPulse(ChanEvent *event_, MapEntry *entry_);

	/// Process an individual events.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL){ return false; }

  public:
	Processor(std::string name_, std::string type_, MapFile *map_);
	
	virtual ~Processor();

	virtual void GetHists(std::vector<Plotter*> &plots_){ }

	std::string GetType(){ return type; }
	
	std::string GetName(){ return name; }

	unsigned long GetNumTotalEvents(){ return total_events; }

	unsigned long GetNumHandledEvents(){ return total_handled; }

	unsigned long GetNumUnprocessed(){ return (handle_notValid+handle_unpairedEvent); }
	
	bool IsInit(){ return init; }
	
	bool ToggleTraces(){ return (write_waveform = !write_waveform); }
	
	void SetFitting(){ analyzer = FIT; }

	void SetTraditionalCFD(){ analyzer = CFD; }

	void SetPolynomialCFD(){ analyzer = POLY; }

	void SetTraceAnalyzer(TimingAnalyzer mode_){ analyzer = mode_; }

	void SetDefaultCfdParameters(const float &F_, const float &D_=1, const float &L_=1){ defaultCFD[0] = F_; defaultCFD[1] = D_; defaultCFD[2] = L_; }
	
	bool Initialize(TTree *tree_);
	
	bool InitializeTraces(TTree *tree_);

	float Status(unsigned long global_events_);

	void AddEvent(ChannelEventPair *event_){ events.push_back(event_); }
	
	void PreProcess();

	bool Process(ChannelEventPair *start_);
	
	/// Finish processing of events by clearing the event list.
	void WrapUp();
	
	void Zero();

	void RemoveByTag(const std::string &tag_, const bool &withTag_=true);
};

#endif
