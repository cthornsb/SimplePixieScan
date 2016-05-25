#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>

// PixieCore libraries
#include "Unpacker.hpp"
#include "ScanInterface.hpp"

#include "Structures.h"

class MapFile;
class ConfigFile;
class ProcessorHandler;
class OnlineProcessor;
class Plotter;

class TFile;
class TTree;

class simpleUnpacker : public Unpacker {
  public:
  	/// Default constructor.
	simpleUnpacker() : Unpacker() {  }
	
	/// Destructor.
	~simpleUnpacker(){  }
	
  private:
	/** Process all events in the event list.
	  * \param[in]  addr_ Pointer to a location in memory. Unused by default.
	  * \return Nothing.
	  */
	virtual void ProcessRawEvent(void *addr_=NULL);
	
	/** Add an event to generic statistics output.
	  * \param[in]  event_ Pointer to the current XIA event. Unused by default.
	  * \return Nothing.
	  */
	virtual void RawStats(XiaEvent *event_);
};

class simpleScanner : public ScanInterface {
  public:
  	/// Default constructor.
	simpleScanner();
	
	/// Destructor.
	~simpleScanner();

	/** Search for an input command and perform the desired action.
	  * \param[in]  cmd_ The command to interpret. Not used by default.
	  * \param[out] args_ Vector or arguments to the user command. Not used by default.
	  * \return True if the command was recognized and false otherwise. Returns false by default.
	  */
	virtual bool ExtraCommands(const std::string &cmd_, std::vector<std::string> &args_);
	
	/** Scan input arguments and set class variables.
	  */
	virtual bool ExtraArguments(const std::string &arg_, std::deque<std::string> &others_, std::string &ifname);
	
	/** Print an in-terminal help dialogue for recognized commands.
	  * \param[in]  prefix_ String to append at the start of any output. Not used by default.
	  * \return Nothing.
	  */
	virtual void CmdHelp();
	
	/** Print a command line help dialogue for recognized command line arguments.
	  * \return Nothing.
	  */
	virtual void ArgHelp();
	
	/** Print the usage string for this program.
	  */
	virtual void SyntaxStr(char *name_);

	/** IdleTask is called whenever a scan is running in shared
	  * memory mode, and a spill has yet to be received. This method may
	  * be used to update things which need to be updated every so often
	  * (e.g. a root TCanvas) when working with a low data rate. 
	  * Does nothing useful by default.
	  * \return Nothing.
	  */
	virtual void IdleTask(){  }

	/** Initialize the map file, the config file, the processor handler, 
	  * and add all of the required processors.
	  */
	virtual bool Initialize(std::string prefix_="");
	
	/** Peform any last minute initialization before processing data. 
	  * /return Nothing.
	  */
	virtual void FinalInitialization();
	
	/** Initialize the root output. 
	  * Does nothing useful by default.
	  * \param[in]  fname_     Filename of the output root file. Not used by default.
	  * \param[in]  overwrite_ Set to true if the user wishes to overwrite the output file. Not used by default.
	  * \return True upon successfully opening the output file and false otherwise. Returns false by default.
	  */
	virtual bool InitRootOutput(std::string fname_, bool overwrite_=true){ return false; }

	/** Receive various status notifications from the scan.
	  * \param[in] code_ The notification code passed from ScanInterface methods. Not used by default.
	  * \return Nothing.
	  */
	virtual void Notify(const std::string &code_="");

	/** Return a pointer to the Unpacker object to use for data unpacking.
	  * If no object has been initialized, create a new one.
	  * \return Pointer to an Unpacker object.
	  */
	virtual Unpacker *GetCore();

	/** Add a channel event to the deque of events to send to the processors.
	  * This method should only be called from simpleUnpacker::ProcessRawEvent().
	  * \param[in]  event_ The raw XiaEvent to add to the channel event deque.
	  * \return Nothing.
	  */
	void AddEvent(XiaEvent *event_);
	
	/** Process all channel events read in from the rawEvent.
	  * This method should only be called from simpleUnpacker::ProcessRawEvent().
	  * \return Nothing.
	  */
	void ProcessEvents();

  private:
	MapFile *mapfile;
	ConfigFile *configfile;
	ProcessorHandler *handler;
	OnlineProcessor *online;
	
	TFile *root_file; /// Output root file for storing data.
	TTree *root_tree; /// Output TTree for storing processed data.
	TTree *trace_tree; /// Output TTree for storing raw ADC traces.
	TTree *raw_tree; /// Output TTree for storing raw pixie data.
	
	Plotter *chanCounts; /// 2d root histogram to store number of total channel counts found.
	Plotter *chanEnergy; /// 2d root histogram to store the energy spectra from all channels.
	
	int events_since_last_update; /// The number of processed events since the last online histogram update.
	int events_between_updates; /// The number of events to process before updating online histograms.
	
	int loaded_files; /// The number of files which have been processed.
	
	int raw_event_module; /// Module ID from the channel event.
	int raw_event_channel; /// Channel ID from the channel event.
	double raw_event_energy; /// Raw pixie energy taken directly from the module (a.u.).
	double raw_event_time; /// Raw pixie time taken directly from the module and converted to seconds.
	
	bool force_overwrite;
	bool online_mode;
	bool use_root_fitting;
	bool write_traces;
	bool init;
	
	std::string output_filename;
};

#endif
