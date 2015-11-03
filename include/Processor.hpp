#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <deque>

#include "TF1.h"

class ChannelEvent;
class MapFile;

class TTree;
class TBranch;
class TF1;

class Processor{
  protected:
	clock_t start_time;
	unsigned long total_time;
	
	unsigned long good_events;
	
	ChannelEvent *start;
	std::deque<ChannelEvent*> events;
	
	std::string name;
	std::string type;
	bool init;
	bool write_waveform;
	bool use_color_terminal;
	bool hires_timing;
	
	TBranch *local_branch;
	TF1 *fitting_func;
	
	MapFile *mapfile;

	const double clockInSeconds = 8e-9; /// One pixie clock is 8 ns
	const double adcClockInSeconds = 4e-9; /// One ADC clock is 4 ns
	const double filterClockInSeconds = 8e-9; /// One filter clock is 8 ns
  
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
	
	void SetFitFunction(double (*func_)(double *, double *), int npar_);
	
	void SetFitFunction(const char* func_);

	virtual void SetFitParameters(double *data);

	virtual void FitPulses();

	virtual bool HandleEvents();

  public:
	Processor(std::string name_, std::string type_, MapFile *map_);
	
	virtual ~Processor();
	
	std::string GetType(){ return type; }
	
	std::string GetName(){ return name; }
	
	TF1 *GetFunction(){ return fitting_func; }
	
	bool IsInit(){ return init; }
	
	bool SetHiResMode(bool state_=true){ return (hires_timing = state_); }
	
	virtual bool Initialize(TTree *tree_);

	float Status(unsigned long total_events_);

	void AddEvent(ChannelEvent *event_){ events.push_back(event_); }

	void PreProcess();

	bool Process(ChannelEvent *start_);
	
	virtual void Zero(){ }
};

#endif
