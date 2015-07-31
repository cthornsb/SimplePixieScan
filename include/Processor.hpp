#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <deque>

struct ChannelEvent;

class Processor{
  protected:
	clock_t start_time;
	unsigned long total_time;
	
	unsigned long good_events;
	unsigned long total_events;
	
	ChannelEvent *start;
	std::deque<ChannelEvent*> events;
	
	std::string name;
	std::string type;
	bool init;
	bool write_waveform;
  
	/// Clear channel events from the queue
	void ClearEvents();
  
	/// Start the process timer
	void StartProcess(){ start_time = clock(); }
	
	/// Update the amount of time taken by the processor
	void StopProcess(){ total_time += (clock() - start_time); }
	
	virtual bool HandleEvents();

  public:
	Processor(std::string name_, std::string type_);
	
	~Processor();

	std::string GetName(){ return name; }
	
	bool IsInit(){ return init; }

	float Status();

	void AddEvent(ChannelEvent *event_){ events.push_back(event_); }

	bool Process(ChannelEvent *start_);
};

#endif
