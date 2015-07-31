#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>
#include <queue>

struct ChannelEvent;

class Processor{
  protected:
	clock_t start_time;
	unsigned long total_time;
	
	unsigned long good_events;
	unsigned long total_events;
	
	ChannelEvent *start;
	std::queue<ChannelEvent*> events;
	
	std::string name;
	bool init;
  
	/// Clear channel events from the queue
	void ClearEvents();
  
	/// Start the process timer
	void StartProcess(){ start_time = clock(); }
	
	/// Update the amount of time taken by the processor
	void StopProcess(){ total_time += (clock() - start_time); }
	
	virtual bool HandleEvents();

  public:
	Processor(std::string name_="Generic");
	
	~Processor();

	std::string GetName(){ return name; }
	
	bool IsInit(){ return init; }

	float Status(unsigned int total_events);

	void AddEvent(ChannelEvent *event_){ events.push(event_); }

	bool Process(ChannelEvent *start_);
};

#endif
