#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>

// PixieCore libraries
#include "Unpacker.hpp"
#include "ScanInterface.hpp"

// Root libraries
#include "TTree.h"

#include "Structures.h"

class MapFile;
class ConfigFile;
class CalibFile;
class ProcessorHandler;
class OnlineProcessor;
class Plotter;

class TFile;
class TCanvas;

///////////////////////////////////////////////////////////////////////////////
// class extTree
///////////////////////////////////////////////////////////////////////////////

class extTree : public TTree {
  private:
	bool doDraw; /// Set to true when the user has requested a histogram to be drawn.
	std::string expr; /// User specified expression to draw using TTree::Draw().
	std::string gate; /// User specified selection criteria string to use for drawing.
	std::string opt; /// User specified root drawing option string.

	TCanvas *canvas; /// Canvas for plotting histograms.

	/** Open a TCanvas if this tree has not already done so.
	  * \return Pointer to an open TCanvas.
	  */
	TCanvas *OpenCanvas();

  public:
	/** Named constructor.
	  *	\param[in]  name_ Name of the underlying TTree.
	  * \param[in]  title_ Title of the underlying TTree.
	  */
	extTree(const char *name_, const char *title_);

	/// Destructor.
	~extTree();
	
	/** Safely draw from the underlying TTree using TTree::Draw(). Must preceed calls to SafeDraw().
	  * \param[in]  expr_ Root TFormula expression to draw from the TTree.
	  * \param[in]  gate_ Root selection string to use for plotting.
	  * \param[in]  opt_ Root drawing option.
	  * \return True if the tree is ready to draw and false if the tree is already waiting to draw.
	  */
	bool SafeDraw(const std::string &expr_, const std::string &gate_="", const std::string &opt_="");
	
	/** Safely fill the tree and draw a histogram using TTree::Draw() if required.
	  * \return The return value from TTree::Draw().
	  */
	int SafeFill();
};

///////////////////////////////////////////////////////////////////////////////
// class simpleUnpacker
///////////////////////////////////////////////////////////////////////////////

class simpleUnpacker : public Unpacker {
  public:
  	/// Default constructor.
	simpleUnpacker();
	
	/// Destructor.
	~simpleUnpacker(){  }
	
	extTree *GetTree(){ return stat_tree; }

	/** Initialize the raw event statistics tree.
	  * \return Pointer to the TTree.
	  */
	extTree *InitTree();
	
  private:
	int raw_event_mult; /// The multiplicity of the current raw event.
	double raw_event_start; /// The start time of the current raw event.
	double raw_event_stop; /// The stop time of the current raw event.
	double raw_event_btwn; /// The time since the previous raw event ended.

	extTree *stat_tree; /// Output TTree for storing low-level statistics.

	/** Process all events in the event list.
	  * \param[in]  addr_ Pointer to a ScanInterface object.
	  * \return Nothing.
	  */
	virtual void ProcessRawEvent(ScanInterface *addr_=NULL);
	
	/** Add an event to generic statistics output.
	  * \param[in]  event_ Pointer to the current XIA event.
	  * \param[in]  addr_  Pointer to a ScanInterface object.
	  * \return Nothing.
	  */
	virtual void RawStats(XiaData *event_, ScanInterface *addr_=NULL){  }
};

///////////////////////////////////////////////////////////////////////////////
// class simpleScanner
///////////////////////////////////////////////////////////////////////////////

class simpleScanner : public ScanInterface {
  public:
  	/// Default constructor.
	simpleScanner();
	
	/// Destructor.
	~simpleScanner();

	/** ExtraCommands is used to send command strings to classes derived
	  * from ScanInterface. If ScanInterface receives an unrecognized
	  * command from the user, it will pass it on to the derived class.
	  * \param[in]  cmd_ The command to interpret.
	  * \param[out] arg_ Vector or arguments to the user command.
	  * \return True if the command was recognized and false otherwise.
	  */
	virtual bool ExtraCommands(const std::string &cmd_, std::vector<std::string> &args_);
	
	/** ExtraArguments is used to send command line arguments to classes derived
	  * from ScanInterface. If ScanInterface receives an unrecognized
	  * argument from the user, it will pass it on to the derived class.
	  * \param[in]  arg_    The argument to interpret.
	  * \param[out] others_ The remaining arguments following arg_.
	  * \param[out] ifname  The input filename to send back to use for reading.
	  * \return True if the argument was recognized and false otherwise.
	  */
	virtual bool ExtraArguments(const std::string &arg_, std::deque<std::string> &others_, std::string &ifname);
	
	/** CmdHelp is used to allow a derived class to print a help statement about
	  * its own commands. This method is called whenever the user enters 'help'
	  * or 'h' into the interactive terminal (if available).
	  * \param[in]  prefix_ String to append at the start of any output. Not used by default.
	  * \return Nothing.
	  */
	virtual void CmdHelp(const std::string &prefix_="");
	
	/** ArgHelp is used to allow a derived class to print a help statment about
	  * its own command line arguments. This method is called at the end of
	  * the ScanInterface::help method.
	  * \return Nothing.
	  */
	virtual void ArgHelp();
	
	/** SyntaxStr is used to print a linux style usage message to the screen.
	  * \param[in]  name_ The name of the program.
	  * \return Nothing.
	  */
	virtual void SyntaxStr(char *name_);

	/** IdleTask is called whenever a scan is running in shared
	  * memory mode, and a spill has yet to be received. This method may
	  * be used to update things which need to be updated every so often
	  * (e.g. a root TCanvas) when working with a low data rate. 
	  * \return Nothing.
	  */
	virtual void IdleTask();

	/** Initialize the map file, the config file, the processor handler, 
	  * and add all of the required processors.
	  * \param[in]  prefix_ String to append to the beginning of system output.
	  * \return True upon successfully initializing and false otherwise.
	  */
	virtual bool Initialize(std::string prefix_="");
	
	/** Peform any last minute initialization before processing data. 
	  * /return Nothing.
	  */
	virtual void FinalInitialization();
	
	/** Initialize the root output. 
	  * \param[in]  fname_     Filename of the output root file. 
	  * \param[in]  overwrite_ Set to true if the user wishes to overwrite the output file. 
	  * \return True upon successfully opening the output file and false otherwise. 
	  */
	virtual bool InitRootOutput(std::string fname_, bool overwrite_=true){ return false; }

	/** Receive various status notifications from the scan.
	  * \param[in] code_ The notification code passed from ScanInterface methods.
	  * \return Nothing.
	  */
	virtual void Notify(const std::string &code_="");

	/** Return a pointer to the Unpacker object to use for data unpacking.
	  * If no object has been initialized, create a new one.
	  * \return Pointer to an Unpacker object.
	  */
	virtual Unpacker *GetCore();

	/** Add a channel event to the deque of events to send to the processors.
	  * This method should only be called from Unpacker::ProcessRawEvent().
	  * \param[in]  event_ The raw XiaData to add.
	  * \return True if the event is added to the processor handler, and false otherwise.
	  */
	virtual bool AddEvent(XiaData *event_);
	
	/** Process all channel events read in from the rawEvent.
	  * This method should only be called from Unpacker::ProcessRawEvent().
	  * \return True if at least one valid signal was found, and false otherwise.
	  */
	virtual bool ProcessEvents();

  private:
	MapFile *mapfile; /// Pointer to the map file to use for channel mapping.
	ConfigFile *configfile; /// Pointer to the configuration file to use for setting default parameters.
	CalibFile *calibfile; /// Pointer to the energy calibration file.
	ProcessorHandler *handler; /// Pointer to the processor handler to use for controlling detector processors.
	OnlineProcessor *online; /// Pointer to the online processor to use for online plotting.
	
	TFile *root_file; /// Output root file for storing data.
	extTree *root_tree; /// Output TTree for storing processed data.
	extTree *trace_tree; /// Output TTree for storing raw ADC traces.
	extTree *raw_tree; /// Output TTree for storing raw pixie data.
	extTree *stat_tree; /// Output TTree for storing low-level statistics.
	
	Plotter *chanCounts; /// 2d root histogram to store number of total channel counts found.
	Plotter *chanMaxADC; /// 2d root histogram to store the energy spectra from all channels.
	
	int events_since_last_update; /// The number of processed events since the last online histogram update.
	int events_between_updates; /// The number of events to process before updating online histograms.
	
	int loaded_files; /// The number of files which have been processed.
	
	int xia_data_module; /// Module ID from the channel event.
	int xia_data_channel; /// Channel ID from the channel event.
	double xia_data_energy; /// Raw pixie energy taken directly from the module (a.u.).
	double xia_data_time; /// Raw pixie time taken directly from the module and converted to seconds.
	
	bool force_overwrite; /// Set to true if existing output files will be overwritten.
	bool online_mode; /// Set to true if online mode is to be used.
	bool use_root_fitting; /// Set to true if root TF1 fitting is to be used for trace analysis.
	bool write_traces; /// Set to true if ADC traces are to be written to the output file.
	bool write_raw; /// Set to true if raw pixie module data is to be written to the output file.
	bool write_stats; /// Set to true if event builder information is to be written to the output file.
	bool init; /// Set to true when the initialization process successfully completes.
	
	std::string output_filename; /// Path to the output root file.
	std::string head_path;
};

#endif
