#include <iostream>

// Local files
#include "Scanner.hpp"
#include "MapFile.hpp"
#include "ConfigFile.hpp"
#include "Processor.hpp"
#include "ProcessorHandler.hpp"
#include "OnlineProcessor.hpp"
#include "Plotter.hpp"
#include "ColorTerm.hpp"

#ifdef USE_HRIBF
#include "ScanorInterface.hpp"
#endif

// Root libraries
#include "TFile.h"
#include "TH1.h"
#include "TNamed.h"
#include "TCanvas.h"
#include "TSystem.h"

// Define the name of the program.
#if not defined(PROG_NAME)
#define PROG_NAME "Scanner"
#endif

template <typename T>
void writeTNamed(const char *label_, const T &val_, const int &precision_=-1){
	std::stringstream stream; 
	if(precision_ < 0) 
		stream << val_;
	else{ // Set the precision of the output stream.
		stream.precision(precision_);
		stream << std::fixed << val_;
	}
	TNamed named(label_, stream.str().c_str());
	named.Write();
}

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

///////////////////////////////////////////////////////////////////////////////
// class extTree
///////////////////////////////////////////////////////////////////////////////

TCanvas *extTree::OpenCanvas(){
	if(!canvas){
		std::string canvasName = std::string(this->GetName())+"_c1";
		std::string canvasTitle = std::string(this->GetName()) + " canvas";
		canvas = new TCanvas(canvasName.c_str(), canvasTitle.c_str());
	}
	
	return canvas;
}

extTree::extTree(const char *name_, const char *title_) : TTree(name_, title_), doDraw(false), expr(), gate(), opt(), canvas(NULL) {
}

extTree::~extTree(){
	if(canvas){
		canvas->Close();
		delete canvas;
	}
}

void extTree::SafeDraw(const std::string &expr_, const std::string &gate_/*=""*/, const std::string &opt_/*=""*/){
	expr = expr_;
	gate = gate_;
	opt = opt_;
	doDraw = true;
}

void extTree::SafeDraw(){
	doDraw = true;
}

int extTree::SafeFill(const bool &doFill/*=true*/){
	int retval = -1;
	if(doFill)
		retval = this->Fill();
	if(doDraw){
		if(GetEntries() > 0){ // Check if there are entries in the tree
			if(!expr.empty()){ // Check if there is a string to draw
				OpenCanvas()->cd();
				std::cout << " draw: " << this->Draw(expr.c_str(), gate.c_str(), opt.c_str()) << std::endl;
				canvas->Update();
			}
			else{ // The draw string is empty, nothing to draw
				std::cout << " Refusing draw of empty string\n";
			}
		}
		else{ // The tree has no entries. Drawing on an empty tree causes seg-faults occasionally
			std::cout << " Refusing draw from tree which contains no entries\n";
		}
		doDraw = false;
	}
	return retval;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleUnpacker
///////////////////////////////////////////////////////////////////////////////

/// Default constructor.
simpleUnpacker::simpleUnpacker() : Unpacker() {  
	stat_tree = NULL;
}

XiaData *simpleUnpacker::GetNewEvent(){ 
	return (XiaData*)(new ChanEvent()); 
}

void simpleUnpacker::ProcessRawEvent(ScanInterface *addr_/*=NULL*/){
	if(!addr_ || rawEvent.empty()){ return; }
	
	// Low-level raw event statistics information.
	if(stat_tree) stat_tree->SafeFill();

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

extTree *simpleUnpacker::InitTree(){
	// Setup the stats tree for data output.
	stat_tree = new extTree("stats", "Low-level statistics tree");

	// Add branches to the stats tree.
	stat_tree->Branch("trig", GetStartEventTime());
	stat_tree->Branch("start", GetRawEventStartTime());
	stat_tree->Branch("stop", GetRawEventStopTime());
	stat_tree->Branch("chanTime", GetRawEventChanTime());
	stat_tree->Branch("chanID", GetRawEventChanID());
	stat_tree->Branch("inEvent", GetRawEventFlag());

	return stat_tree;
}

///////////////////////////////////////////////////////////////////////////////
// class simpleScanner
///////////////////////////////////////////////////////////////////////////////

simpleScanner::simpleScanner() : ScanInterface() {
	recordAllStarts = false;
	nonStartEvents = false;
	firstEvent = true;
	forceUseOfTrace = false;
	untriggered_mode = false;
	force_overwrite = false;
	online_mode = false;
	use_root_fitting = false;
	use_traditional_cfd = false;
	write_traces = false;
	write_raw = false;
	write_stats = false;
	init = false;
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
	online = NULL;
	spillThreshold = 10000;
	currSpillLength = 0;
	maxSpillLength = 0;
	events_since_last_update = 0;
	events_between_updates = 5000;
	loaded_files = 0;
	defaultCFDparameter = -1;
}

simpleScanner::~simpleScanner(){
	if(init){
		std::cout << msgHeader << "Found " << chanCounts->GetHist()->GetEntries() << " total events.\n";

		// If the root file is open, write the tree and histograms.
		if(online_mode) // Write all online diagnostic histograms to the output root file.
			std::cout << msgHeader << "Writing " << online->WriteHists(root_file) << " histograms to root file.\n";

		// Add total data time to the file.
		root_file->cd(head_path.c_str());
		writeTNamed("Data time", handler->GetDeltaEventTime(), 1);

		// Add data start time to the file.
		writeTNamed("Start time", handler->GetFirstEventTime(), 1);

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

		// Write processor count statistics.
		root_file->mkdir("counts");
		root_file->cd("counts");

		writeTNamed("Global", chanCounts->GetHist()->GetEntries());
		writeTNamed("Handled", handler->GetTotalEvents());
		writeTNamed("Starts", handler->GetStartEvents());

		for(size_t i = 0; i < handler->GetNumProcessors(); i++){
			ProcessorEntry *ptr = handler->GetProcessor(i);
			std::stringstream stream; stream << "counts/" << ptr->type;
			root_file->mkdir(stream.str().c_str());
			root_file->cd(stream.str().c_str());
			writeTNamed("Total", ptr->proc->GetNumTotalEvents());
			writeTNamed("Handled", ptr->proc->GetNumHandledEvents());
			writeTNamed("Unhandled", ptr->proc->GetNumUnprocessed());
		}

		// Close the root file.
		root_file->Close();
		delete root_file;
		
		std::cout << msgHeader << "Processed " << loaded_files << " files.\n";
		std::cout << msgHeader << "Found " << handler->GetTotalEvents() << " events.\n";
		if(!untriggered_mode) std::cout << msgHeader << "Found " << handler->GetStartEvents() << " start events.\n";
		std::cout << msgHeader << "Total data time is " << handler->GetDeltaEventTime() << std::endl;
	
		delete mapfile;
		delete configfile;
		delete handler;
		delete online;
	}
}

bool simpleScanner::ExtraCommands(const std::string &cmd_, std::vector<std::string> &args_){
	if(online_mode){
		if(cmd_ == "refresh"){
			if(args_.size() >= 1){
				int frequency = strtol(args_.at(0).c_str(), NULL, 10);
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
		else if(cmd_ == "clear"){
			online->Clear();
		}
		else if(cmd_ == "set"){
			if(args_.size() >= 2){
				int index1 = strtol(args_.at(0).c_str(), NULL, 10);
				int index2 = strtol(args_.at(1).c_str(), NULL, 10);
				if(args_.size() >= 3){
					int detID = strtol(args_.at(2).c_str(), NULL, 10);
					if(online->ChangeHist(index1, index2, detID))
						std::cout << msgHeader << "Set TPad " << index1 << " to histogram '" << index2 << "', detID=" << detID << ".\n";
					else
						std::cout << msgHeader << "Failed to set TPad " << index1 << " to histogram '" << index2 << "', detID=" << detID << "!\n";
				}
				else if(online->ChangeHist(index1, args_.at(1))){ std::cout << msgHeader << "Set TPad " << index1 << " to histogram '" << args_.at(1) << "'.\n"; }
				else if(online->ChangeHist(index1, index2)){ std::cout << msgHeader << "Set TPad " << index1 << " to histogram ID " << index2 << ".\n"; }
				else{ std::cout << msgHeader << "Failed to set TPad " << index1 << " to histogram '" << args_.at(1) << "'!\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'set'\n";
				std::cout << msgHeader << " -SYNTAX- set <pad> <hist> [detector]\n";
			}
		}
		else if(cmd_ == "setopt"){
			if(args_.size() >= 1){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				std::string drawopt = (args_.size() >= 2 ? args_.at(1) : "");
				if(online->SetDrawOpt(index, drawopt))
					std::cout << msgHeader << "Set TPad " << index << " draw opt \"" << drawopt << "\"\n";
				else
					std::cout << msgHeader << "Failed to set TPad " << index << " draw opt!\n";
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'setopt'\n";
				std::cout << msgHeader << " -SYNTAX- setopt <pad> [opt]\n";
			}
		}
		else if(cmd_ == "xlog"){
			if(args_.size() >= 1){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				if(online->ToggleLogX(index)){ std::cout << msgHeader << "Successfully toggled x-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle x-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'xlog'\n";
				std::cout << msgHeader << " -SYNTAX- xlog <pad>\n";
			}
		}
		else if(cmd_ == "ylog"){
			if(args_.size() >= 1){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				if(online->ToggleLogY(index)){ std::cout << msgHeader << "Successfully toggled y-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle y-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'ylog'\n";
				std::cout << msgHeader << " -SYNTAX- ylog <pad>\n";
			}
		}
		else if(cmd_ == "zlog"){
			if(args_.size() >= 1){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				if(online->ToggleLogZ(index)){ std::cout << msgHeader << "Successfully toggled z-axis log scale for TPad " << index << ".\n"; }
				else{ std::cout << msgHeader << "Failed to toggle z-axis log scale for TPad " << index << ".\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'zlog'\n";
				std::cout << msgHeader << " -SYNTAX- zlog <pad>\n";
			}
		}
		else if(cmd_ == "xrange"){
			if(args_.size() >= 3){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				double min = strtod(args_.at(1).c_str(), NULL);
				double max = strtod(args_.at(2).c_str(), NULL);
				if(max > min){ 
					if(online->SetXrange(index, min, max)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ std::cout << msgHeader << "Invalid range for x-axis <" << min << ", " << max << ">\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'xrange'\n";
				std::cout << msgHeader << " -SYNTAX- xrange <pad> <min> <max>\n";
			}
		}
		else if(cmd_ == "yrange"){
			if(args_.size() >= 3){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				double min = strtod(args_.at(1).c_str(), NULL);
				double max = strtod(args_.at(2).c_str(), NULL);
				if(max > min){ 
					if(online->SetYrange(index, min, max)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ std::cout << msgHeader << "Invalid range for y-axis <" << min << ", " << max << ">\n"; }
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'yrange'\n";
				std::cout << msgHeader << " -SYNTAX- yrange <pad> <min> <max>\n";
			}
		}
		else if(cmd_ == "unzoom"){
			if(args_.size() >= 1){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
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
				std::cout << msgHeader << " -SYNTAX- unzoom <pad> [axis]\n";
			}
		}
		else if(cmd_ == "range"){
			if(args_.size() >= 5){
				int index = strtol(args_.at(0).c_str(), NULL, 10);
				double xmin = strtod(args_.at(1).c_str(), NULL);
				double xmax = strtod(args_.at(2).c_str(), NULL);
				double ymin = strtod(args_.at(3).c_str(), NULL);
				double ymax = strtod(args_.at(4).c_str(), NULL);
				if(xmax > xmin && ymax > ymin){ 
					if(online->SetRange(index, xmin, xmax, ymin, ymax)){ std::cout << msgHeader << "Successfully set range of TPad " << index << ".\n"; }
					else{ std::cout << msgHeader << "Failed to set range of TPad " << index << "!\n"; }
				}
				else{ 
					if(xmax <= xmin && ymax <= ymin){
						std::cout << msgHeader << "Invalid range for x-axis <" << xmin << ", " << xmax;
						std::cout << "> and y-axis <" << ymin << ", " << ymax << ">\n"; 
					}
					else if(xmax <= xmin){ std::cout << msgHeader << "Invalid range for x-axis <" << xmin << ", " << xmax << ">\n"; }
					else{ std::cout << msgHeader << "Invalid range for y-axis <" << ymin << ", " << ymax << ">\n"; }
				}
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'range'\n";
				std::cout << msgHeader << " -SYNTAX- range <pad> <xmin> <xmax> <ymin> <ymax>\n";
			}
		}
		else if(cmd_ == "draw"){
			if(args_.size() >= 1){
				std::string gateStr = (args_.size() >= 3)?args_.at(2):"";
				std::string optStr = (args_.size() >= 4)?args_.at(3):"";
				if(args_.size() >= 2)
					std::cout << " " << args_.at(0) << "->Draw(\"" << args_.at(1) << "\", \"" << gateStr << "\", \"" << optStr << "\")\n";
				if(args_.at(0) == "data"){
					if(args_.size() >= 2)
						root_tree->SafeDraw(args_.at(1), gateStr, optStr);
					else
						root_tree->SafeDraw();
					if(!GetIsRunning()) // Manually update the histogram
						root_tree->SafeFill(false);
				}
				else if(args_.at(0) == "raw"){
					if(write_raw){
						if(args_.size() >= 2)
							raw_tree->SafeDraw(args_.at(1), gateStr, optStr);
						else
							raw_tree->SafeDraw();
						if(!GetIsRunning()) // Manually update the histogram
							raw_tree->SafeFill(false);
					}
					else
						std::cout << msgHeader << "The \"raw\" tree is not available! Use --raw flag\n";
				}
				else if(args_.at(0) == "stats"){
					if(write_stats){
						if(args_.size() >= 2)
							stat_tree->SafeDraw(args_.at(1), gateStr, optStr);
						else
							stat_tree->SafeDraw();
						if(!GetIsRunning()) // Manually update the histogram
							stat_tree->SafeFill(false);
					}
					else
						std::cout << msgHeader << "The \"stats\" tree is not available! Use --stats flag\n";
				}
				else{
					std::cout << msgHeader << "Invalid TTree specification!\n";
					std::cout << msgHeader << " Available trees include 'data', 'raw', and 'stats'.\n";
				}
			}
			else{
				std::cout << msgHeader << "Invalid number of parameters to 'draw'\n";
				std::cout << msgHeader << " -SYNTAX- draw <data|raw|stats> [expr] [gate] [opt]\n";
			}
		}
		else if(cmd_ == "zero"){
			bool restartScan = false;
			if(GetIsRunning()){ // Stop the scan, if it's running
				stop_scan();
				restartScan = true;
			}
			if(args_.size() >= 1){
				int index1 = strtol(args_.at(0).c_str(), NULL, 10);
				if(online->Zero(index1)){ std::cout << msgHeader << "Zeroed histogram '" << args_.at(0) << "'.\n"; }
				else{ std::cout << msgHeader << "Failed to zero histogram '" << args_.at(0) << "'!\n"; }
			}
			else{
				// Zero all histograms
				for(unsigned int i = 0; i < online->GetNumHistograms(); i++){
					online->Zero(i);
					std::cout << msgHeader << " Zeroed histogram '" << i << "'.\n";
				}
			}
			if(restartScan)
				start_scan();
		}
		else{ return false; }
	}
	else{ return false; }

	return true;
}

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
	if(userOpts.at(4).active){ // Traditional CFD.
		std::cout << msgHeader << "Toggling traditional CFD ON.\n";
		use_traditional_cfd = true;
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
	if(userOpts.at(8).active){ // Record all starts.
		std::cout << msgHeader << "Recording all start events to output file.\n";
		recordAllStarts = true;
	}
	if(userOpts.at(9).active){ // Set default fitting/CFD parameters
		defaultCFDparameter = strtof(userOpts.at(9).argument.c_str(), NULL);
		std::cout << msgHeader << "Set default CFD parameter to " << defaultCFDparameter << ".\n";
	}
	if(userOpts.at(10).active){ // Force trace processing.
		std::cout << msgHeader << "Forcing using of trace processor.\n";
		forceUseOfTrace = true;
	}
	if(userOpts.at(11).active){ // Set output filename prefix.
		outputFilenamePrefix = userOpts.at(11).argument;
		if(outputFilenamePrefix.back() != '/') outputFilenamePrefix += '/';
		std::cout << msgHeader << "Using output filename prefix \"" << outputFilenamePrefix << "\".\n";
	}
}

void simpleScanner::CmdHelp(const std::string &prefix_/*=""*/){
	if(online_mode){
		std::cout << "   refresh [update]           - Set refresh frequency of online diagnostic plots (default=5000).\n";
		std::cout << "   list                       - List all plottable online histograms.\n";
		std::cout << "   clear                      - Clear the canvas.\n";
		std::cout << "   zero [hist]                - Zero a histogram.\n";
		std::cout << "   set <pad> <hist> [ID]      - Set the histogram to draw to part of the canvas.\n";
		std::cout << "   setopt <pad> [opt]         - Set (or un-set) the draw option for a displayed histogram.\n";
		std::cout << "   xlog <pad>                 - Toggle the x-axis log/linear scale of a specified histogram.\n";
		std::cout << "   ylog <pad>                 - Toggle the y-axis log/linear scale of a specified histogram.\n";
		std::cout << "   zlog <pad>                 - Toggle the z-axis log/linear scale of a specified histogram.\n"; 
		std::cout << "   xrange <pad> <min> <max>   - Set the x-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   yrange <pad> <min> <max>   - Set the y-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   unzoom <pad> [x|y]         - Unzoom the x-axis, the y-axis, or both.\n";
		std::cout << "   range <pad> <xmin> <xmax> <ymin> <ymax>   - Set the range of the x and y axes.\n";
		std::cout << "   draw <data|raw|stats> [expr] [gate] [opt] - Draw a histogram using TTree::Draw().\n";
	}
}

void simpleScanner::ArgHelp(){
	AddOption(optionExt("untriggered", no_argument, NULL, 'u', "", "Run without a start detector"));
	AddOption(optionExt("force", no_argument, NULL, 'f', "", "Force overwrite of the output root file"));
	AddOption(optionExt("online", no_argument, NULL, 0, "", "Plot online root histograms for monitoring data"));
	AddOption(optionExt("fitting", no_argument, NULL, 0, "", "Use root fitting for high resolution timing"));
	AddOption(optionExt("cfd", no_argument, NULL, 0, "", "Use traditional CFD for high resolution timing"));
	AddOption(optionExt("traces", no_argument, NULL, 0, "", "Dump raw ADC traces to output root file"));
	AddOption(optionExt("raw", no_argument, NULL, 0, "", "Dump raw pixie module data to output root file"));
	AddOption(optionExt("stats", no_argument, NULL, 0, "", "Dump event builder information to the output root file"));
	AddOption(optionExt("record", no_argument, NULL, 0, "", "Write all start events to output file even when no other events are found"));
	AddOption(optionExt("parameters", required_argument, NULL, 0, "<list>", "Set default fitting/CFD parameters by supplying comma-delimited string"));
	AddOption(optionExt("force-traces", no_argument, NULL, 0, "", "Change all entries in map file to type 'trace' to do trace analysis"));
	AddOption(optionExt("output-prefix", required_argument, NULL, 0, "<prefix>", "Set the output file prefix (default is ./)"));
}

void simpleScanner::SyntaxStr(char *name_){ 
	std::cout << " usage: " << std::string(name_) << " [options]\n"; 
}

void simpleScanner::IdleTask(){
	if(online_mode)
		gSystem->ProcessEvents();
}

bool simpleScanner::Initialize(std::string prefix_){
	if(init){ return false; }

	// Setup a 2d histogram for tracking all channel counts.
	chanCounts = new Plotter("chanCounts", "Recorded Counts for Module vs. Channel", "COLZ", "Channel", "", 16, 0, 16, "Module", "", 6, 0, 6);

	// Setup a 2d histogram for tracking channel energies.
	chanMaxADC = new Plotter("chanMaxADC", "Channel vs. Max ADC", "COLZ", "Max ADC Channel", "", 16384, 0, 16384, "Channel", "", 96, 0, 96);

	// Setup a 2d histogram for tracking channel energies.
	chanEnergy = new Plotter("chanEnergy", "Channel vs. Filter Energy", "COLZ", "Filter Energy", "a.u.", 32768, 0, 32768, "Channel", "", 96, 0, 96);

	std::string setupDirectory = this->GetSetupFilename();
	if(setupDirectory.empty()) setupDirectory = "./setup/";
	else if(setupDirectory.back() != '/') setupDirectory += '/';
	std::cout << prefix_ << "Using setup directory \"" << setupDirectory << "\".\n";

	// Initialize map file, config file, and processor handler.
	std::string currentFile = setupDirectory + "map.dat";
	std::cout << prefix_ << "Reading map file " << currentFile << "\n";
	mapfile = new MapFile(currentFile.c_str());
	if(!mapfile->IsInit()){ // Failed to read map file.
		errStr << prefix_ << "Failed to read map file '" << currentFile << "'.\n";
		delete mapfile;
		return false;
	}

	if(online_mode){
		// Initialize the online data processor.
		online = new OnlineProcessor();

		// Read the histogram map.
		currentFile = setupDirectory + "hist.dat";
		std::cout << prefix_ << "Reading histogram map file " << currentFile << "\n";
		if(!online->ReadHistMap(currentFile.c_str())){
			errStr << prefix_ << "ERROR! Failed to read histogram map file '" << currentFile << "'.\n";
			return false;
		}

		online->SetDisplayMode();
		online->SetMapFile(mapfile);
	
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

	for(int i = 0; i <= mapfile->GetMaxModule(); i++){
		for(int j = 0; j < 16; j++){
			MapEntry *mapptr = mapfile->GetMapEntry(i, j);
			if(!mapptr || mapptr->type == "ignore") continue;
			else if(mapptr->hasTag("untriggered")){ // Add this channel to the unpacker whitelist so that it is always added to the raw event.
				GetCore()->AddToWhitelist(i, j);
				std::cout << prefix_ << "Adding mod=" << i << ", chan=" << j << " to unpacker whitelist.\n";
			}
		}
	}

	if(forceUseOfTrace){
		// Overwrite all map file entries with 'trace' types.
		for(int i = 0; i <= mapfile->GetMaxModule(); i++){
			for(int j = 0; j < 16; j++){
				MapEntry *mapptr = mapfile->GetMapEntry(i, j);
				if(!mapptr || mapptr->type == "ignore") continue;
				mapptr->type = "trace";
				mapptr->subtype = "";
			}
		}
		mapfile->ClearTypeList();
		mapfile->AddTypeToList("trace");
	}
	
	currentFile = setupDirectory + "config.dat";
	std::cout << prefix_ << "Reading config file " << currentFile << "\n";
	configfile = new ConfigFile(currentFile.c_str());
	if(!configfile->IsInit()){ // Failed to read config file.
		errStr << prefix_ << "Failed to read configuration file '" << setupDirectory << "'.\n";
		delete mapfile;
		delete configfile;
		return false;
	}
	
	bool hadErrors = false;
	GetCore()->SetEventWidth(configfile->eventWidth * (1E-6 / configfile->sysClock)); // = eventWidth * 1E-6(s/us) / SYSCLOCK(s/tick)
	GetCore()->SetEventDelay(configfile->eventDelay * (1E-6 / configfile->sysClock)); // = eventDelay * 1E-6(s/us) / SYSCLOCK(s/tick)

	if(untriggered_mode && configfile->buildMethod >= 2){
		warnStr << msgHeader << "Warning! Raw event build method (" << configfile->buildMethod << ") is invalid for untriggered mode.\n";
		configfile->buildMethod = 0;
	}

	GetCore()->SetRawEventMode(configfile->buildMethod);
	std::cout << prefix_ << "Set event width to " << configfile->eventWidth << " μs (" << GetCore()->GetEventWidth() << " pixie clock ticks).\n";
	std::cout << prefix_ << "Set event delay to " << configfile->eventDelay << " μs (" << GetCore()->GetEventDelay() << " pixie clock ticks).\n";
	std::cout << prefix_ << "Set raw event builder mode to (" << configfile->buildMethod << ").\n";
	handler = new ProcessorHandler();
	
	int startMod, startChan;
	if(mapfile->GetFirstStart(startMod, startChan)){
		GetCore()->SetStartChannel(startMod, startChan);
		std::cout << prefix_ << "Set start channel to (" << startMod << ", " << startChan << ").\n";
	}

	// Load all needed processors.
	std::vector<std::string> *types = mapfile->GetTypes();
	for(std::vector<std::string>::iterator iter = types->begin(); iter != types->end(); iter++){
		if(*iter == "ignore" || !handler->CheckProcessor(*iter)){ continue; }
		else{
			Processor *proc = handler->AddProcessor(*iter, mapfile);
			if(proc){
				std::cout << prefix_ << "Added " << *iter << " processor to the processor list.\n"; 
			
				if(online_mode){ // Initialize all online diagnostic plots.
					online->StartAddHists(proc);
					if(online->HadHistError()){
						warnStr << prefix_ << "Disabling plotting for detectors of type " << proc->GetType() << ".\n";
						proc->DisablePlotting();
						hadErrors = true;
					}
				}

				if(defaultCFDparameter > 0)
					proc->SetDefaultCfdParameters(defaultCFDparameter);
				
				// Set the module clock cycles.
				proc->SetAdcClockInSeconds(configfile->adcClock);
				proc->SetSystemClockInSeconds(configfile->sysClock);
			}
			else{
				 
				warnStr << prefix_ << "Failed to add " << *iter << " processor to the processor list!\n"; 
				hadErrors = true;			
			}
		}
	}

	std::cout << prefix_ << "Set module ADC clock to " << configfile->adcClock << " seconds/tick.\n";
	std::cout << prefix_ << "Set module system clock to " << configfile->sysClock << " seconds/tick.\n";

	if(hadErrors){
		std::string userInput;
		while(true){
			warnStr << prefix_ << "Encountered errors during initialization. Continue? (y,n) ";
			std::cin >> userInput;
			if(userInput == "y" || userInput == "Y") break;
			else if(userInput == "n" || userInput == "N") return false; // Hard abort.
			std::cout << prefix_ << "Invalid input (" << userInput << ").\n";
		}
	}
	
	// Get the output filename.
	std::string ofname = GetOutputFilename();

	if(ofname.empty()){
		ofname = GetInputFilename(); 
		size_t index = ofname.find_last_of('/');
		ofname = ofname.substr(index+1);
		index = ofname.find_last_of('.');
		ofname = outputFilenamePrefix + ofname.substr(0, index) + ".root";
		std::cout << prefix_ << "No output filename given, using \"" << ofname << "\".\n";
	}

	// Initialize the root output file.
	std::cout << prefix_ << "Initializing root output.\n";
	if(force_overwrite){ root_file = new TFile(ofname.c_str(), "RECREATE"); }
	else{ root_file = new TFile(ofname.c_str(), "CREATE"); }

	// Check that the root file is open.
	if(!root_file->IsOpen()){
		errStr << prefix_ << "Failed to open output root file '" << ofname << "'!\n";
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

	// Set trace analysis processor.
	if(use_root_fitting) handler->SetTimingAnalyzer(FIT);
	else if(use_traditional_cfd) handler->SetTimingAnalyzer(CFD);
	else handler->SetTimingAnalyzer(POLY);

	// Set untriggered mode.
	if(untriggered_mode){
		GetCore()->SetUntriggeredMode(true);	
		handler->ToggleUntriggered();
	}

	return (init = true);
}

void simpleScanner::FinalInitialization(){
	// Add file header information to the output root file.
	root_file->mkdir("head");

	// Add map and config file entries to the file.	
	mapfile->Write(root_file);
	configfile->Write(root_file);
}

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
				
			// Write header information to the output root file.
			std::stringstream stream;
			if(loaded_files < 10){ stream << "head/file0" << loaded_files; }
			else{ stream << "head/file" << loaded_files; }
			head_path = stream.str();
			root_file->mkdir(head_path.c_str());
			root_file->cd(head_path.c_str());
			for(size_t index = 0; index < finfo->size(); index++){
				finfo->at(index, name, value);
				writeTNamed(name.c_str(), value);
			}
		}
		else{ std::cout << msgHeader << "Failed to fetch input file info!\n"; }
	}
	else if(code_ == "REWIND_FILE"){  }
	else if(code_ == "RESTART"){  }
	else{ std::cout << msgHeader << "Unknown notification code '" << code_ << "'!\n"; }
}

Unpacker *simpleScanner::GetCore(){ 
	if(!core){ core = (Unpacker*)(new simpleUnpacker()); }
	return core;
}

bool simpleScanner::AddEvent(XiaData *event_){
	if(!event_){ return false; }

	if(firstEvent) firstEvent = false; // This is the first event to be processed.

	// Fill the output histograms.
	chanCounts->Fill2d(event_->chanNum, event_->modNum);
	chanEnergy->Fill2d(event_->energy, 16*event_->modNum + event_->chanNum);

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

	// Create a new ChanEvent.	
	current_event = new ChanEvent(event_);
	
	// We no longer need the XiaData since we made a copy of it using ChanEvent.
	delete event_;
	
	// Link the channel event to its corresponding map entry.
	ChannelEventPair *pair_;
	pair_ = new ChannelEventPair(current_event, mapentry);

	// Correct the baseline before using the trace.
	if(pair_->channelEvent->traceLength != 0 && pair_->channelEvent->ComputeBaseline() >= 0.0){
		chanMaxADC->Fill2d(pair_->channelEvent->maximum, pair_->entry->location);
	}
	
	// Pass this event to the correct processor
	if(!handler->AddEvent(pair_)){ // Invalid detector type. Delete it
		delete pair_;
		return false;
	}

	if(!untriggered_mode && pair_->entry->hasTag("start")){ 
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

bool simpleScanner::ProcessEvents(){
	bool retval = true;

	// Check that at least one of the events in the event list is not a
	// start event. This is done to avoid writing a lot of useless data
	// to the output file in the event of a high trigger rate.
	if(nonStartEvents || recordAllStarts){
		// Call each processor to do the processing.
		if(handler->Process()){ // This event had at least one valid signal
			// Fill the root tree with processed data.
			root_tree->SafeFill();

			// Fill the ADC trace tree with raw traces.		
			if(write_traces){ trace_tree->SafeFill(); }
		}
		else{ retval = false; }
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

int main(int argc, char *argv[]){
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
