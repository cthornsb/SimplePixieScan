#include <iostream>

// Local files
#include "Scanner.hpp"
#include "MapFile.hpp"
#include "ConfigFile.hpp"
#include "CalibFile.hpp"
#include "Processor.hpp"
#include "ProcessorHandler.hpp"
#include "OnlineProcessor.hpp"
#include "Plotter.hpp"

#ifdef USE_HRIBF
#include "ScanorInterface.hpp"
#endif

// Root libraries
#include "TFile.h"
#include "TH1.h"
#include "TNamed.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TApplication.h"

// Define the name of the program.
#if not defined(PROG_NAME)
#define PROG_NAME "Scanner"
#endif

const unsigned int fileFooterWord = 0x20464f45; // "EOF "
const unsigned int fileHeaderWord = 0x44414548; // "HEAD"
const unsigned int dataHeaderWord = 0x41544144; // "DATA"
const unsigned int endBufferWord = 0xFFFFFFFF;

void writeFileInfo(std::ofstream &file_, const std::string &str_){
	unsigned short dummy1 = ((unsigned short)str_.size() + 2) + (str_.size() + 2) % 4;
	unsigned short dummy2 = (str_.size() + 2) % 4;
	file_.write((char *)&dummy1, 2);
	file_.write(str_.c_str(), str_.size());
	if(dummy2 > 0){ // Pad with whitespace.
		std::string tempStr(dummy2, ' ');
		file_.write(tempStr.c_str(), dummy2);
	}
}

/** Open a TCanvas if this tree has not already done so.
  * \return Pointer to an open TCanvas.
  */
TCanvas *extTree::OpenCanvas(){
	if(!canvas){
		std::string canvasName = std::string(this->GetName())+"_c1";
		std::string canvasTitle = std::string(this->GetName()) + " canvas";
		canvas = new TCanvas(canvasName.c_str(), canvasTitle.c_str());
	}
	
	return canvas;
}

/** Named constructor.
  *	\param[in]  name_ Name of the underlying TTree.
  * \param[in]  title_ Title of the underlying TTree.
  */
extTree::extTree(const char *name_, const char *title_) : TTree(name_, title_){
	expr = "";
	gate = "";
	opt = "";
	doDraw = false;
	canvas = NULL;
}

/// Destructor.
extTree::~extTree(){
	if(canvas){
		canvas->Close();
		delete canvas;
	}
}

/** Safely draw from the underlying TTree using TTree::Draw(). Must preceed calls to SafeDraw().
  * \param[in]  expr_ Root TFormula expression to draw from the TTree.
  * \param[in]  gate_ Root selection string to use for plotting.
  * \param[in]  opt_ Root drawing option.
  * \return True if the tree is ready to draw and false if the tree is already waiting to draw.
  */
bool extTree::SafeDraw(const std::string &expr_, const std::string &gate_/*=""*/, const std::string &opt_/*=""*/){
	if(doDraw){ return false; }
	
	expr = expr_;
	gate = gate_;
	opt = opt_;
	
	return (doDraw = true);
}

/** Safely fill the tree and draw a histogram using TTree::Draw() if required.
  * \return The return value from TTree::Draw().
  */
int extTree::SafeFill(){
	int retval = this->Fill();

	if(doDraw){
		OpenCanvas()->cd();
		std::cout << " draw: " << this->Draw(expr.c_str(), gate.c_str(), opt.c_str()) << std::endl;
		canvas->Update();
		doDraw = false;
	}

	return retval;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleUnpacker
///////////////////////////////////////////////////////////////////////////////

/// Default constructor.
simpleUnpacker::simpleUnpacker() : Unpacker() {  
	raw_event_mult = 0;
	raw_event_start = 0;
	raw_event_stop = 0;
	raw_event_btwn = 0;
	stat_tree = NULL;
}

/** Return a pointer to a new XiaData channel event.
  * \return A pointer to a new XiaData.
  */
XiaData *simpleUnpacker::GetNewEvent(){ 
	return (XiaData*)(new ChanEvent()); 
}

/** Process all events in the event list.
  * \param[in]  addr_ Pointer to a location in memory. 
  * \return Nothing.
  */
void simpleUnpacker::ProcessRawEvent(ScanInterface *addr_/*=NULL*/){
	if(!addr_ || rawEvent.empty()){ return; }
	
	// Low-level raw event statistics information.
	if(stat_tree){
		raw_event_mult = (int)rawEvent.size();
		raw_event_start = GetRealStartTime();
		if(raw_event_stop != 0) // Get the time since the end of the last raw event.
			raw_event_btwn = raw_event_start - raw_event_stop;
		raw_event_stop = GetRealStopTime();
		stat_tree->SafeFill();
	}

	XiaData *current_event = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		rawEvent.pop_front(); // Remove this event from the raw event deque.
		
		// Check that this channel event exists.
		if(!current_event){ continue; }

		// Send the event to the scan interface object for processing.
		addr_->AddEvent(current_event);
	}
	
	// Finish up with this raw event.
	addr_->ProcessEvents();
}

/** Initialize the raw event statistics tree.
  * \return Pointer to the TTree.
  */
extTree *simpleUnpacker::InitTree(){
	// Setup the stats tree for data output.
	stat_tree = new extTree("stats", "Low-level statistics tree");

	// Add branches to the stats tree.
	stat_tree->Branch("mult", &raw_event_mult);
	stat_tree->Branch("start", &raw_event_start);
	stat_tree->Branch("stop", &raw_event_stop);
	stat_tree->Branch("Tbtwn", &raw_event_btwn);
	
	return stat_tree;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleScanner
///////////////////////////////////////////////////////////////////////////////

/// Default constructor.
simpleScanner::simpleScanner() : ScanInterface() {
	recordAllStarts = false;
	nonStartEvents = false;
	presortData = false;
	firstEvent = true;
	writePresort = false;
	use_calibrations = true;
	untriggered_mode = false;
	force_overwrite = false;
	online_mode = false;
	use_root_fitting = false;
	write_traces = false;
	write_raw = false;
	write_stats = false;
	init = false;
	mapfile = NULL;
	configfile = NULL;
	calibfile = NULL;
	handler = NULL;
	online = NULL;
	spillThreshold = 10000;
	currSpillLength = 0;
	maxSpillLength = 0;
	events_since_last_update = 0;
	events_between_updates = 5000;
	loaded_files = 0;
}

/// Destructor.
simpleScanner::~simpleScanner(){
	if(init){
		std::cout << msgHeader << "Found " << chanCounts->GetHist()->GetEntries() << " total events.\n";

		// Get the total acquisition time.
		std::stringstream stream;
		stream << handler->GetDeltaEventTime() << " s";

		// If the root file is open, write the tree and histogram.
		if(!writePresort && root_file->IsOpen()){
			// Write all online diagnostic histograms to the output root file.
			std::cout << msgHeader << "Writing " << online->WriteHists(root_file) << " histograms to root file.\n";

			// Add total data time to the file.
			root_file->cd(head_path.c_str());
			TNamed named("Data time", stream.str().c_str());
			named.Write();
			
			root_file->cd();

			// Write root trees to output file.
			std::cout << msgHeader << "Writing " << root_tree->GetEntries() << " processed data entries to root file.\n";
			root_tree->Write();			
			
			if(write_raw){
				std::cout << msgHeader << "Writing " << raw_tree->GetEntries() << " raw data entries to root file.\n";
				raw_tree->Write();
			}
			
			if(write_traces){
				std::cout << msgHeader << "Writing " << trace_tree->GetEntries() << " raw ADC traces to root file.\n";
				trace_tree->Write();
			}
			
			if(write_stats){
				std::cout << msgHeader << "Writing " << stat_tree->GetEntries() << " raw event stats entries to root file.\n";
				stat_tree->Write();
			}
			
			// Write debug histograms.
			chanCounts->GetHist()->Write();
			chanMaxADC->GetHist()->Write();
			chanEnergy->GetHist()->Write();
			
			// Close the root file.
			root_file->Close();
			delete root_file;
		}
		else if(writePresort){
			// Write the remaining sorted data to the output file.
			HandlePresortOutput(true);

			// Close the file.
			psort_file.write((char *)&fileFooterWord, 4);
			psort_file.write((char *)&endBufferWord, 4);
		
			// Write the maximum raw event spill length and the total time to the header.
			PLD_header tempHeader;
			tempHeader.SetMaxSpillSize(maxSpillLength);
			tempHeader.SetRunTime(handler->GetDeltaEventTime());
			tempHeader.OverwriteValues(&psort_file);
		
			std::cout << msgHeader << "Wrote " << psort_file.tellp() << " B to output file.\n";
			
			// Close the presort file.
			psort_file.close();
		}

		std::cout << msgHeader << "Processed " << loaded_files << " files.\n";
		std::cout << msgHeader << "Found " << handler->GetTotalEvents() << " events.\n";
		if(!untriggered_mode) std::cout << msgHeader << "Found " << handler->GetStartEvents() << " start events.\n";
		std::cout << msgHeader << "Total data time is " << stream.str() << std::endl;
	
		delete mapfile;
		delete configfile;
		delete calibfile;
		delete handler;
		delete online;
	}
}

/** ExtraCommands is used to send command strings to classes derived
  * from ScanInterface. If ScanInterface receives an unrecognized
  * command from the user, it will pass it on to the derived class.
  * \param[in]  cmd_ The command to interpret.
  * \param[out] arg_ Vector or arguments to the user command.
  * \return True if the command was recognized and false otherwise.
  */
bool simpleScanner::ExtraCommands(const std::string &cmd_, std::vector<std::string> &args_){
	if(online_mode){
		if(cmd_ == "refresh"){
			if(args_.size() >= 1){
				int frequency = atoi(args_.at(0).c_str());
				if(frequency > 0){ 
					std::cout << msgHeader << "Set canvas update frequency to " << frequency << " events.\n"; 
					events_between_updates = frequency;
				}
				else{ std::cout << msgHeader << "Failed to set canvas update frequency to " << frequency << " events!\n"; }
			}
			else{ online->Refresh(); }
		}
		else if(cmd_ == "list"){
			online->PrintHists();
		}
		else if(cmd_ == "set"){
			if(args_.size() >= 2){
				int index1 = atoi(args_.at(0).c_str());
				int index2 = atoi(args_.at(1).c_str());
				if(online->ChangeHist(index1, args_.at(1))){ std::cout << msgHeader << "Set TPad " << index1 << " to histogram '" << args_.at(1) << "'.\n"; }
				else if(online->ChangeHist(index1, index2)){ std::cout << msgHeader << "Set TPad " << index1 << " to histogram ID " << index2 << ".\n"; }
				else{ std::cout << msgHeader << "Failed to set TPad " << index1 << " to histogram '" << args_.at(1) << "'!\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'set'\n";
				std::cout << msgHeader << " -SYNTAX- set [index] [hist]\n";
			}
		}
		else if(cmd_ == "xlog"){
			if(args_.size() >= 1){
				int index = atoi(args_.at(0).c_str());
				if(online->ToggleLogX(index)){ std::cout << msgHeader << "Successfully toggled x-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle x-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'xlog'\n";
				std::cout << msgHeader << " -SYNTAX- xlog [index]\n";
			}
		}
		else if(cmd_ == "ylog"){
			if(args_.size() >= 1){
				int index = atoi(args_.at(0).c_str());
				if(online->ToggleLogY(index)){ std::cout << msgHeader << "Successfully toggled y-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle y-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'ylog'\n";
				std::cout << msgHeader << " -SYNTAX- ylog [index]\n";
			}
		}
		else if(cmd_ == "zlog"){
			if(args_.size() >= 1){
				int index = atoi(args_.at(0).c_str());
				if(online->ToggleLogZ(index)){ std::cout << msgHeader << "Successfully toggled z-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle z-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'zlog'\n";
				std::cout << msgHeader << " -SYNTAX- zlog [index]\n";
			}
		}
		else if(cmd_ == "xrange"){
			if(args_.size() >= 3){
				int index = atoi(args_.at(0).c_str());
				float min = atof(args_.at(1).c_str());
				float max = atof(args_.at(2).c_str());
				if(max > min){ 
					if(online->SetXrange(index, min, max)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ std::cout << msgHeader << "Invalid range for x-axis [" << min << ", " << max << "]\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'xrange'\n";
				std::cout << msgHeader << " -SYNTAX- xrange [index] [min] [max]\n";
			}
		}
		else if(cmd_ == "yrange"){
			if(args_.size() >= 3){
				int index = atoi(args_.at(0).c_str());
				float min = atof(args_.at(1).c_str());
				float max = atof(args_.at(2).c_str());
				if(max > min){ 
					if(online->SetYrange(index, min, max)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ std::cout << msgHeader << "Invalid range for y-axis [" << min << ", " << max << "]\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'yrange'\n";
				std::cout << msgHeader << " -SYNTAX- yrange [index] [min] [max]\n";
			}
		}
		else if(cmd_ == "unzoom"){
			if(args_.size() >= 1){
				int index = atoi(args_.at(0).c_str());
				if(args_.size() >= 2){
					if(args_.at(1) == "x" || args_.at(1) == "X" || args_.at(1) == "0"){
						online->ResetXrange(index);
						std::cout << msgHeader << "Reset range of X axis.\n";
					}
					else if(args_.at(1) == "y" || args_.at(1) == "Y" || args_.at(1) == "1"){
						online->ResetYrange(index);
						std::cout << msgHeader << "Reset range of Y axis.\n";
					}
					else{ std::cout << msgHeader << "Unknown axis (" << args_.at(1) << ")!\n"; }
				}
				else{
					online->ResetRange(index);
					std::cout << msgHeader << "Reset range of X and Y axes.\n";
				}
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'unzoom'\n";
				std::cout << msgHeader << " -SYNTAX- unzoom [index] <axis>\n";
			}
		}
		else if(cmd_ == "range"){
			if(args_.size() >= 5){
				int index = atoi(args_.at(0).c_str());
				float xmin = atof(args_.at(1).c_str());
				float xmax = atof(args_.at(2).c_str());
				float ymin = atof(args_.at(3).c_str());
				float ymax = atof(args_.at(4).c_str());
				if(xmax > xmin && ymax > ymin){ 
					if(online->SetRange(index, xmin, xmax, ymin, ymax)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ 
					if(xmax <= xmin && ymax <= ymin){
						std::cout << msgHeader << "Invalid range for x-axis [" << xmin << ", " << xmax;
						std::cout << "] and y-axis [" << ymin << ", " << ymax << "]\n"; 
					}
					else if(xmax <= xmin){ std::cout << msgHeader << "Invalid range for x-axis [" << xmin << ", " << xmax << "]\n"; }
					else{ std::cout << msgHeader << "Invalid range for y-axis [" << ymin << ", " << ymax << "]\n"; }
				}
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'range'\n";
				std::cout << msgHeader << " -SYNTAX- range [index] [xmin] [xmax] [ymin] [ymax]\n";
			}
		}
		else if(cmd_ == "draw"){
			if(args_.size() >= 2){
				std::string gateStr = (args_.size() >= 3)?args_.at(2):"";
				std::string optStr = (args_.size() >= 4)?args_.at(3):"";
				if(args_.at(0) == "data")
					root_tree->SafeDraw(args_.at(1), gateStr, optStr);
				else if(args_.at(0) == "raw")
					raw_tree->SafeDraw(args_.at(1), gateStr, optStr);
				else if(args_.at(0) == "stats")
					stat_tree->SafeDraw(args_.at(1), gateStr, optStr);
				else{
					std::cout << msgHeader << "Invalid TTree specification!\n";
					std::cout << msgHeader << " Available trees include 'data', 'raw', and 'stats'.\n";
				}
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'draw'\n";
				std::cout << msgHeader << " -SYNTAX- draw <tree> <expr> [gate] [opt]\n";
			}
		}
		else if(cmd_ == "zero"){
			if(args_.size() >= 1){
				int index1 = atoi(args_.at(0).c_str());
				if(online->Zero(index1)){ std::cout << msgHeader << "Zeroed histogram '" << args_.at(0) << "'.\n"; }
				else{ std::cout << msgHeader << "Failed to zero histogram '" << args_.at(0) << "'!\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'zero'\n";
				std::cout << msgHeader << " -SYNTAX- zero <hist>\n";
			}
		}
		else{ return false; }
	}
	else{ return false; }

	return true;
}

/** ExtraArguments is used to send command line arguments to classes derived
  * from ScanInterface. This method should loop over the optionExt elements
  * in the vector userOpts and check for those options which have been flagged
  * as active by ::Setup(). This should be overloaded in the derived class.
  * \return Nothing.
  */
void simpleScanner::ExtraArguments(){
	if(userOpts.at(0).active){ // Untriggered.
		std::cout << msgHeader << "Using untriggered mode.\n";
		untriggered_mode = true;	
	}
	if(userOpts.at(1).active){ // Force overwrite.
		std::cout << msgHeader << "Forcing overwrite of output file.\n";
		force_overwrite = true;	
	}
	if(userOpts.at(2).active){ // Online mode.
		std::cout << msgHeader << "Using online mode.\n";
		online_mode = true;	
	}
	if(userOpts.at(3).active){ // Root fitting.
		std::cout << msgHeader << "Toggling root fitting ON.\n";
		use_root_fitting = true;	
	}
	if(userOpts.at(4).active){ // Uncalibrated mode.
		std::cout << msgHeader << "Using uncalibrated mode.\n";
		use_calibrations = false;	
	}
	if(userOpts.at(5).active){ // Traces.
		std::cout << msgHeader << "Toggling ADC trace output ON.\n";
		write_traces = true;	
	}
	if(userOpts.at(6).active){ // Raw.
		std::cout << msgHeader << "Writing raw pixie data to output tree.\n";
		write_raw = true;	
	}
	if(userOpts.at(7).active){ // Stats.
		std::cout << msgHeader << "Writing event builder stats to output tree.\n";
		write_stats = true;		
	}
	if(userOpts.at(8).active){ // Presort.
		std::cout << msgHeader << "Using presort mode.\n";
		writePresort = true;	
	}
	if(userOpts.at(9).active){ // Record all starts.
		std::cout << msgHeader << "Recording all start events to output file.\n";
		recordAllStarts = true;
	}
}

/** CmdHelp is used to allow a derived class to print a help statement about
  * its own commands. This method is called whenever the user enters 'help'
  * or 'h' into the interactive terminal (if available).
  * \param[in]  prefix_ String to append at the start of any output. Not used by default.
  * \return Nothing.
  */
void simpleScanner::CmdHelp(const std::string &prefix_/*=""*/){
	if(online_mode){
		std::cout << "   refresh <update>           - Set refresh frequency of online diagnostic plots (default=5000).\n";
		std::cout << "   list                       - List all plottable online histograms.\n";
		std::cout << "   zero <hist>                - Zero a histogram.\n";
		std::cout << "   set <index> <hist>         - Set the histogram to draw to part of the canvas.\n";
		std::cout << "   xlog <index>               - Toggle the x-axis log/linear scale of a specified histogram.\n";
		std::cout << "   ylog <index>               - Toggle the y-axis log/linear scale of a specified histogram.\n";
		std::cout << "   zlog <index>               - Toggle the z-axis log/linear scale of a specified histogram.\n"; 
		std::cout << "   xrange <index> <min> <max> - Set the x-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   yrange <index> <min> <max> - Set the y-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   unzoom <index> [axis]      - Unzoom the x-axis, the y-axis, or both.\n";
		std::cout << "   range <index> <xmin> <xmax> <ymin> <ymax> - Set the range of the x and y axes.\n";
		std::cout << "   draw <tree> <expr> [gate] [opt]           - Draw a histogram using TTree::Draw().\n";
	}
}

/** ArgHelp is used to allow a derived class to add a command line option
  * to the main list of options. This method is called at the end of
  * from the ::Setup method.
  * Does nothing useful by default.
  * \return Nothing.
  */
void simpleScanner::ArgHelp(){
	AddOption(optionExt("untriggered", no_argument, NULL, 'u', "", "Run without a start detector"));
	AddOption(optionExt("force", no_argument, NULL, 'f', "", "Force overwrite of the output root file"));
	AddOption(optionExt("online", no_argument, NULL, 0, "", "Plot online root histograms for monitoring data"));
	AddOption(optionExt("fitting", no_argument, NULL, 0, "", "Use root fitting for high resolution timing"));
	AddOption(optionExt("uncal", no_argument, NULL, 0, "", "Do not calibrate channel energies"));
	AddOption(optionExt("traces", no_argument, NULL, 0, "", "Dump raw ADC traces to output root file"));
	AddOption(optionExt("raw", no_argument, NULL, 0, "", "Dump raw pixie module data to output root file"));
	AddOption(optionExt("stats", no_argument, NULL, 0, "", "Dump event builder information to the output root file"));
	AddOption(optionExt("presort", no_argument, NULL, 'P', "", "Write an intermediate binary output file containing presorted data"));
	AddOption(optionExt("record", no_argument, NULL, 0, "", "Write all start events to output file even when no other events are found"));
}

/** SyntaxStr is used to print a linux style usage message to the screen.
  * \param[in]  name_ The name of the program.
  * \return Nothing.
  */
void simpleScanner::SyntaxStr(char *name_){ 
	std::cout << " usage: " << std::string(name_) << " [options]\n"; 
}

/** IdleTask is called whenever a scan is running in shared
  * memory mode, and a spill has yet to be received. This method may
  * be used to update things which need to be updated every so often
  * (e.g. a root TCanvas) when working with a low data rate. 
  * \return Nothing.
  */
void simpleScanner::IdleTask(){
	gSystem->ProcessEvents();
}

/** Initialize the map file, the config file, the processor handler, 
  * and add all of the required processors.
  * \param[in]  prefix_ String to append to the beginning of system output.
  * \return True upon successfully initializing and false otherwise.
  */
bool simpleScanner::Initialize(std::string prefix_){
	if(init){ return false; }

	// Setup a 2d histogram for tracking all channel counts.
	chanCounts = new Plotter("chanCounts", "Recorded Counts for Module vs. Channel", "COLZ", "Channel", 16, 0, 16, "Module", 6, 0, 6);

	// Setup a 2d histogram for tracking channel energies.
	chanMaxADC = new Plotter("chanMaxADC", "Channel vs. Max ADC", "COLZ", "Max ADC Channel", 4096, 0, 4096, "Channel", 96, 0, 96);

	// Setup a 2d histogram for tracking channel energies.
	chanEnergy = new Plotter("chanEnergy", "Channel vs. Filter Energy", "COLZ", "Filter Energy", 4096, 0, 32768, "Channel", 96, 0, 96);

	// Initialize the online data processor.
	online = new OnlineProcessor();

	if(online_mode){
		online->SetDisplayMode();
	
		// Add the raw histograms to the online processor.
		online->AddHist(chanCounts);
		online->AddHist(chanMaxADC);
		online->AddHist(chanEnergy);
	
		// Set the first and second histograms to channel count histogram and energy histogram.
		online->ChangeHist(0, 0);
		online->ChangeHist(1, 1);
		online->ChangeHist(2, 2);
		online->Refresh();
	}

	std::string setupDirectory = this->GetSetupFilename();
	if(setupDirectory.empty()) setupDirectory = "./setup/";
	else if(setupDirectory.back() != '/') setupDirectory += '/';
	std::cout << prefix_ << "Using setup directory \"" << setupDirectory << "\".\n";

	// Initialize map file, config file, and processor handler.
	std::string currentFile = setupDirectory + "map.dat";
	std::cout << prefix_ << "Reading map file " << currentFile << "\n";
	mapfile = new MapFile(currentFile.c_str());
	if(!mapfile->IsInit()){ // Failed to read map file.
		std::cout << prefix_ << "Failed to read map file '" << setupDirectory << "'.\n";
		delete mapfile;
		return false;
	}
	currentFile = setupDirectory + "config.dat";
	std::cout << prefix_ << "Reading config file " << currentFile << "\n";
	configfile = new ConfigFile(currentFile.c_str());
	if(!configfile->IsInit()){ // Failed to read config file.
		std::cout << prefix_ << "Failed to read configuration file '" << setupDirectory << "'.\n";
		delete mapfile;
		delete configfile;
		return false;
	}
	
	calibfile = new CalibFile();
	currentFile = setupDirectory + "time.cal";
	std::cout << prefix_ << "Reading time calibration file " << currentFile << "\n";
	if(!calibfile->LoadTimeCal(currentFile.c_str())) // Failed to read time calibration file.
		std::cout << prefix_ << "Failed to read time calibration file '" << setupDirectory << "'.\n";
	
	currentFile = setupDirectory + "energy.cal";
	std::cout << prefix_ << "Reading energy calibration file " << currentFile << "\n";
	if(!calibfile->LoadEnergyCal(currentFile.c_str())) // Failed to read energy calibration file.
		std::cout << prefix_ << "Failed to read energy calibration file '" << setupDirectory << "'.\n";
	
	currentFile = setupDirectory + "position.cal";
	std::cout << prefix_ << "Reading position calibration file " << currentFile << "\n";
	if(!calibfile->LoadPositionCal(currentFile.c_str())) // Failed to read position calibration file.
		std::cout << prefix_ << "Failed to read position calibration file '" << setupDirectory << "'.\n";
	
	GetCore()->SetEventWidth(configfile->eventWidth * 125); // = eventWidth * 1E-6(s/us) / 8E-9(s/tick)
	std::cout << prefix_ << "Setting event width to " << configfile->eventWidth << " μs (" << GetCore()->GetEventWidth() << " pixie clock ticks).\n";
	handler = new ProcessorHandler();
	
	// Load all needed processors.
	std::vector<MapEntry> *types = mapfile->GetTypes();
	for(std::vector<MapEntry>::iterator iter = types->begin(); iter != types->end(); iter++){
		if(iter->type == "ignore" || !handler->CheckProcessor(iter->type)){ continue; }
		else{
			Processor *proc = handler->AddProcessor(iter->type, mapfile);
			if(proc){
				std::cout << prefix_ << "Added " << iter->type << " processor to the processor list.\n"; 
				
				// Initialize all online diagnostic plots.
				online->AddHists(proc);
			}
			else{ std::cout << prefix_ << "Failed to add " << iter->type << " processor to the processor list!\n"; }
		}
	}
	
	if(!writePresort){
		// Initialize the root output file.
		std::cout << prefix_ << "Initializing root output.\n";
		if(force_overwrite){ root_file = new TFile(GetOutputFilename().c_str(), "RECREATE"); }
		else{ root_file = new TFile(GetOutputFilename().c_str(), "CREATE"); }

		// Check that the root file is open.
		if(!root_file->IsOpen()){
			std::cout << prefix_ << "Failed to open output root file '" << GetOutputFilename() << "'!\n";
			root_file->Close();
			delete root_file;
			root_file = NULL;
			return false;
		}
		
		// Setup the root tree for data output.
		root_tree = new extTree("data", "Pixie data");
	
		// Setup the raw data tree for output.
		if(write_raw){
			raw_tree = new extTree("raw", "Raw pixie data");
	
			// Add branches to the xia data tree.
			raw_tree->Branch("loc", &xia_data_location);
			raw_tree->Branch("energy", &xia_data_energy);
			raw_tree->Branch("time", &xia_data_time);
		}

		// Initialize the unpacker tree.
		if(write_stats)
			stat_tree = ((simpleUnpacker*)GetCore())->InitTree();

		// Add branches to the output tree.
		handler->InitRootOutput(root_tree);		

		// Set processor options.
		if(write_traces){ 
			trace_tree = new extTree("trace", "Raw pixie ADC traces");
			handler->InitTraceOutput(trace_tree); 
		}
	}
	else{
		// Initialize the presort file.
		std::cout << prefix_ << "Initializing presort output file.\n";
		psort_file.open(GetOutputFilename().c_str());
	}

	// Set processor options.
	if(use_root_fitting){ handler->ToggleFitting(); }

	// Set untriggered mode.
	if(untriggered_mode)
		handler->ToggleUntriggered();

	return (init = true);
}

/** Peform any last minute initialization before processing data. 
  * /return Nothing.
  */
void simpleScanner::FinalInitialization(){
	if(!writePresort){
		// Add file header information to the output root file.
		root_file->mkdir("head");
	
		// Add map and config file entries to the file.	
		mapfile->Write(root_file);
		configfile->Write(root_file);

		// Add calibration entries to the file.
		calibfile->Write(root_file);
	}
	else{
	}
}

/** Receive various status notifications from the scan.
  * \param[in] code_ The notification code passed from ScanInterface methods.
  * \return Nothing.
  */
void simpleScanner::Notify(const std::string &code_/*=""*/){
	if(code_ == "START_SCAN"){  }
	else if(code_ == "STOP_SCAN"){  }
	else if(code_ == "SCAN_COMPLETE"){ std::cout << msgHeader << "Scan complete.\n"; }
	else if(code_ == "LOAD_FILE"){
		std::cout << msgHeader << "File loaded.\n";
		fileInformation *finfo = GetFileInfo();
		if(finfo){
			loaded_files++;
			std::string name, value;
			if(!writePresort){
				// Write header information to the output root file.
				std::stringstream stream;
				if(loaded_files < 10){ stream << "head/file0" << loaded_files; }
				else{ stream << "head/file" << loaded_files; }
				head_path = stream.str();
				root_file->mkdir(head_path.c_str());
				root_file->cd(head_path.c_str());
				for(size_t index = 0; index < finfo->size(); index++){
					finfo->at(index, name, value);
					TNamed named(name.c_str(), value.c_str());
					named.Write();
				}
			}
			else{
				// Write header information to the output presort file.
				if(GetFileFormat() == 0){
					PLD_header tempHeader;
					tempHeader.SetFacility(std::string(GetLdfHeader()->GetFacility()));
					tempHeader.SetFormat("PRESORTED_EVENTS");
					tempHeader.SetStartDateTime(std::string(GetLdfHeader()->GetDate()));
					tempHeader.SetEndDateTime(std::string(GetLdfHeader()->GetDate()));					
					tempHeader.SetTitle(std::string(GetLdfHeader()->GetRunTitle()));
					tempHeader.SetRunNumber(GetLdfHeader()->GetRunNumber());
					tempHeader.Write(&psort_file);
				}
				else{
					GetPldHeader()->SetFormat("PRESORTED_EVENTS");
					GetPldHeader()->Write(&psort_file);
				}
			}
		}
		else{ std::cout << msgHeader << "Failed to fetch input file info!\n"; }
	}
	else if(code_ == "REWIND_FILE"){  }
	else{ std::cout << msgHeader << "Unknown notification code '" << code_ << "'!\n"; }
}

/** Return a pointer to the Unpacker object to use for data unpacking.
  * If no object has been initialized, create a new one.
  * \return Pointer to an Unpacker object.
  */
Unpacker *simpleScanner::GetCore(){ 
	if(!core){ core = (Unpacker*)(new simpleUnpacker()); }
	return core;
}

/** Add a channel event to the deque of events to send to the processors.
  * This method should only be called from Unpacker::ProcessRawEvent().
  * \param[in]  event_ The raw XiaData to add.
  * \return True if the event is added to the processor handler, and false otherwise.
  */
bool simpleScanner::AddEvent(XiaData *event_){
	if(!event_){ return false; }

	if(firstEvent){ // This is the first event to be processed.
		if(this->GetFileFormat() == 2){ // Reading presorted data from file.
			handler->SetPresortMode(true);
			presortData = true;
		}
		
		firstEvent = false;
	}

	// Fill the output histograms.
	chanCounts->Fill(event_->chanNum, event_->modNum);
	chanEnergy->Fill(event_->energy, 16*event_->modNum + event_->chanNum);

	// Raw event information. Dump raw event information to root file.
	if(write_raw){
		xia_data_location = 16*event_->modNum + event_->chanNum;
		xia_data_energy = event_->energy;
		xia_data_time = event_->time*8E-9;
		raw_tree->SafeFill();
	}

	// Check that this channel is defined in the map.
	MapEntry *mapentry = mapfile->GetMapEntry(event_);
	if(!mapentry || mapentry->type == "ignore"){
		delete event_;
		return false;
	}
	
	ChanEvent *current_event = NULL;
	
	if(!presortData){ // Non presorted data. Create a new ChanEvent.
		current_event = new ChanEvent(event_);
		
		// We no longer need the XiaData since we made a copy of it using ChanEvent.
		delete event_;
	}
	else{ // We already have a ChanEvent. Convert XiaData pointer to ChanEvent pointer.
		current_event = (ChanEvent*)event_;
	}
	
	// Link the channel event to its corresponding map entry.
	ChannelEventPair *pair_;
	if(use_calibrations)
		pair_ = new ChannelEventPair(current_event, mapentry, calibfile->GetCalibEntry(event_));
	else
		pair_ = new ChannelEventPair(current_event, mapentry, &dummyCalib);

	// Correct the baseline before using the trace.
	if(pair_->channelEvent->traceLength != 0 && pair_->channelEvent->ComputeBaseline() >= 0.0){
		chanMaxADC->Fill(pair_->channelEvent->maximum, pair_->entry->location);
	}
	
	// Pass this event to the correct processor
	if(!handler->AddEvent(pair_)){ // Invalid detector type. Delete it
		delete pair_;
		return false;
	}

	if(!untriggered_mode && pair_->entry->tag == "start"){ 
		// This channel is a start signal. Due to the way ScanList
		// packs the raw event, there may be more than one start signal
		// per raw event.
		handler->AddStart(pair_);
	}
	else{
		// The event list has at least one non-start event.
		nonStartEvents = true;
	}

	// Add this event to the list of all events.
	chanEventList.push_back(pair_);
	
	return true;
}

/** Process all channel events read in from the rawEvent.
  * This method should only be called from Unpacker::ProcessRawEvent().
  * \return True if at least one valid signal was found, and false otherwise.
  */
bool simpleScanner::ProcessEvents(){
	bool retval = true;

	// Check that at least one of the events in the event list is not a
	// start event. This is done to avoid writing a lot of useless data
	// to the output file in the event of a high trigger rate.
	if(nonStartEvents || recordAllStarts){
		if(!writePresort){
			// Call each processor to do the processing.
			if(handler->Process()){ // This event had at least one valid signal
				// Fill the root tree with processed data.
				root_tree->SafeFill();

				// Fill the ADC trace tree with raw traces.		
				if(write_traces){ trace_tree->SafeFill(); }
			}
			else{ retval = false; }
		}
		else{
			// Call each processor's preprocess routine.
			// The preprocessors will calculate high res timing, energy, etc.
			handler->PreProcess();

			// Write the sorted data to the output file.
			HandlePresortOutput();
		}
		
		nonStartEvents = false;
	}

	// Zero all of the processors.
	handler->ZeroAll();

	// Clear all events from the channel event list.
	while(!chanEventList.empty()){
		delete chanEventList.front();
		chanEventList.pop_front(); // Remove this event from the raw event deque.
	}

	// Check for the need to update the online canvas.
	if(online_mode){
		if(events_since_last_update >= events_between_updates){
			online->Refresh();
			events_since_last_update = 0;
		}
		else{ events_since_last_update++; }
	}
	
	return retval;
}

/** Write pixie events in the raw event to the output presort file.
  * \param[in]  forceWrite Close the raw event spill even if the threshold has not been reached.
  */
void simpleScanner::HandlePresortOutput(bool forceWrite/*=false*/){
	// This is a new raw event spill.
	if(currSpillLength == 0){
		// The 1-word DATA identifier.
		psort_file.write((char *)&dataHeaderWord, 4);
		
		// This is where we would write the length of the raw event "spill".
		// Since we don't currently know how long it will be we will use a placeholder for now.
		// We will save the index so we may overwrite it with the correct value later.
		spillLengthIndex = psort_file.tellp();
		
		// Write a 0xFFFFFFFF as a placeholder.
		psort_file.write((char *)&endBufferWord, 4);
		
		// Update the current length of our raw event spill.
		// We will need to subtract these two words later, but we
		// need this now to prevent writing the header more than once.
		currSpillLength += 2; 
	}

	// If there are events in the raw event, write it to the file.
	if(!chanEventList.empty()){
		// Write the event data.
		for(std::deque<ChannelEventPair*>::iterator iter = chanEventList.begin(); iter != chanEventList.end(); ++iter){ 
			// Write each event to the presort file.
			(*iter)->channelEvent->writeEvent(&psort_file, NULL);

			// Add the length of the event.
			currSpillLength += (*iter)->channelEvent->getEventLength();
		}

		// Close the raw event with a delimiter.
		psort_file.write((char *)&endBufferWord, 4);
	
		// Account for the delimiter.
		currSpillLength++;
	}

	// Check if we have enough data to close the spill.
	if(currSpillLength >= spillThreshold + 2 || forceWrite){
		// Close the spill with a delimiter.
		psort_file.write((char *)&endBufferWord, 4);

		// Remove the length of the header from the spill length.
		currSpillLength = currSpillLength - 2;
		
		// Overwrite the 1-word raw event spill length in 4-byte words.
		psort_file.seekp(spillLengthIndex, std::ios::beg);
		psort_file.write((char *)&currSpillLength, 4);
		psort_file.seekp(0, std::ios::end);

		// Keep track of the maximum raw event length. Required for later scanning.
		if(currSpillLength > maxSpillLength)
			maxSpillLength = currSpillLength;
	
		// Reset the spill length.
		currSpillLength = 0;
	}
}

#ifndef USE_HRIBF
int main(int argc, char *argv[]){
	// Initialize root graphics
	TApplication *rootapp = new TApplication("rootapp", 0, NULL);
	
	// This is done to keep the compiler from complaining about
	// unused TApplication variable.
	rootapp->SetBit(0, false);

	// Define a new unpacker object.
	simpleScanner scanner;
	
	// Set the output message prefix.
	scanner.SetProgramName(std::string(PROG_NAME));	
	
	// Initialize the scanner.
	if(!scanner.Setup(argc, argv))
		return 1;

	// Run the main loop.
	int retval = scanner.Execute();
	
	scanner.Close();

	return retval;
}
#else
Unpacker *pixieUnpacker = NULL;
simpleScanner *scanner = NULL;

// Do some startup stuff.
extern "C" void startup_()
{
	scanner = new simpleScanner();	

	// Handle command line arguments.
	scanner->Setup(fortargc, fortargv); // Getting these from scanor...
	
	// Get a pointer to a class derived from Unpacker.
	pixieUnpacker = scanner->GetCore();
}

// Catch the exit call from scanor and clean up c++ objects CRT
extern "C" void cleanup_()
{
	// Do some cleanup.
	std::cout << "\nCleaning up..\n";
	scanner->Close();
	delete scanner;
}
#endif
