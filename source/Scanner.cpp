#include <iostream>

// PixieCore libraries
#include "Unpacker.hpp"
#include "ScanMain.hpp"

// Local files
#include "Scanner.hpp"
#include "MapFile.hpp"
#include "ConfigFile.hpp"
#include "Processor.hpp"
#include "ProcessorHandler.hpp"
#include "OnlineProcessor.hpp"
#include "Plotter.hpp"

// Root libraries
#include "TFile.h"
#include "TTree.h"
#include "TH2I.h"

/// Process all events in the event list.
void Scanner::ProcessRawEvent(){
	ChannelEvent *current_event = NULL;
	ChannelEventPair *current_pair = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		rawEvent.pop_front(); // Remove this event from the raw event deque.
		
		// Check that this channel event exists.
		if(!current_event){ continue; }

		// Fill the output histograms.
		chanCounts->Fill(current_event->chanNum, current_event->modNum);
		chanEnergy->Fill(current_event->energy, current_event->modNum*16+current_event->chanNum);

		if(!raw_event_mode){ // Standard operation. Individual processors will handle output
			// Link the channel event to its corresponding map entry.
			current_pair = new ChannelEventPair(current_event, mapfile->GetMapEntry(current_event));
		
			// Pass this event to the correct processor
			if(current_pair->entry->type == "ignore" || !handler->AddEvent(current_pair)){ // Invalid detector type. Delete it
				delete current_pair;
			}
		
			// This channel is a start signal. Due to the way ScanList
			// packs the raw event, there may be more than one start signal
			// per raw event.
			if(current_pair->entry->tag == "start"){ 
				handler->AddStart(current_pair);
			}
		}
		else{ // Raw event mode operation. Dump raw event information to root file.
			structure.Append(current_event->modNum, current_event->chanNum, current_event->time, current_event->energy);
			waveform.Append(current_event->trace);
			delete current_pair;
		}
	}
	
	if(!raw_event_mode){
		// Call each processor to do the processing. Each
		// processor will remove the channel events when finished.
		if(handler->Process()){
			// This event had at least one valid signal
			root_tree->Fill();
		}
		
		// Zero all of the processors.
		handler->ZeroAll();
	}
	else{
		root_tree->Fill();
		structure.Zero();
		waveform.Zero();
	}
}

Scanner::Scanner(){
	force_overwrite = false;
	raw_event_mode = false;
	use_root_fitting = true;
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
	online = NULL;
	output_filename = "out.root";
}

Scanner::~Scanner(){
	if(init){
		std::cout << "Scanner: Found " << chanCounts->GetHist()->GetEntries() << " total events.\n";
		if(!raw_event_mode){
			std::cout << "Scanner: Found " << handler->GetTotalEvents() << " start events.\n";
			std::cout << "Scanner: Total data time is " << handler->GetDeltaEventTime() << " s.\n";
		
			delete mapfile;
			delete configfile;
			delete handler;
		}
		
		// If the root file is open, write the tree and histogram.
		if(root_file->IsOpen()){
			std::cout << "Scanner: Writing " << root_tree->GetEntries() << " entries to root file.\n";
			root_file->cd();
			root_tree->Write();
			chanCounts->GetHist()->Write();
			chanEnergy->GetHist()->Write();
			root_file->Close();
		}
		delete root_file;
		
		if(online){ delete online; }
	}

	Close(); // Close the Unpacker object.
}

/// Initialize the map file, the config file, the processor handler, and add all of the required processors.
bool Scanner::Initialize(std::string prefix_){
	if(init){ return false; }

	// Initialize the online data processor.
	online = new OnlineProcessor();

	// Setup a 2d histogram for tracking all channel counts.
	chanCounts = new Plotter("chanCounts", "Recorded Counts for Module vs. Channel", "COLZ", "Channel", 16, 0, 16, "Module", 14, 0, 14);

	// Setup a 2d histogram for tracking all channel counts.
	chanEnergy = new Plotter("chanEnergy", "Channel vs. Energy", "COLZ", "Energy (a.u.)", 500, 0, 20000, "Channel", 224, 0, 224);

	// Add the raw histograms to the online processor.
	online->AddHist(chanCounts);
	online->AddHist(chanEnergy);
	
	// Set the first and second histograms to channel count histogram and energy histogram.
	online->ChangeHist(0, 0);
	online->ChangeHist(1, 1);
	online->Refresh();

	// Only initialize map file, config file, and processor handler if not in raw event mode.
	if(!raw_event_mode){
		std::cout << prefix_ << "Reading map file ./setup/map.dat\n";
		mapfile = new MapFile("./setup/map.dat");
		std::cout << prefix_ << "Reading config file ./setup/config.dat\n";
		configfile = new ConfigFile("./setup/config.dat");
		event_width = configfile->event_width * 125; // = event_width * 1E-6(s/us) / 8E-9(s/tick)
		std::cout << prefix_ << "Setting event width to " << configfile->event_width << " μs (" << event_width << " pixie clock ticks).\n";
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
	}
	
	// Initialize the root output file.
	std::cout << prefix_ << "Initializing root output.\n";
	if(force_overwrite){ root_file = new TFile(output_filename.c_str(), "RECREATE"); }
	else{ root_file = new TFile(output_filename.c_str(), "CREATE"); }

	// Check that the root file is open.
	if(!root_file->IsOpen()){
		std::cout << prefix_ << "Failed to open output root file '" << output_filename << "'!\n";
		root_file->Close();
		delete root_file;
		root_file = NULL;
		return false;
	}

	// Setup the root tree for data output.
	root_tree = new TTree("Pixie16", "Pixie16 data");
	
	if(!raw_event_mode){ // Standard operation
		handler->InitRootOutput(root_tree);
		if(!use_root_fitting){ handler->ToggleFitting(); }
	}
	else{ // Raw event output only. Processors will not be called.
		root_tree->Branch("RawEvent", &structure);
		root_tree->Branch("RawWave", &waveform);
	}

	return (init = true);
}

/// Return the syntax string for this program.
void Scanner::SyntaxStr(const char *name_, std::string prefix_){ 
	std::cout << prefix_ << "SYNTAX: " << std::string(name_) << " <options> <input>\n"; 
}
	
/**
 * \param[in] prefix_
 */
void Scanner::ArgHelp(std::string prefix_){
	std::cout << prefix_ << "--force-overwrite - Force an overwrite of the output root file if it exists (default=false)\n";
	std::cout << prefix_ << "--raw-event-mode  - Write raw channel information to the output root file (default=false)\n";
	std::cout << prefix_ << "--no-fitting      - Do not use root fitting for high resolution timing (default=true)\n";
}

/** 
 *	\param[in] prefix_ 
 */
void Scanner::CmdHelp(std::string prefix_){
	std::cout << prefix_ << "refresh                    - Update online diagnostic plots.\n";
	std::cout << prefix_ << "list                       - List all plottable online histograms.\n";
	std::cout << prefix_ << "set [index] [hist]         - Set the histogram to draw to part of the canvas.\n";
	std::cout << prefix_ << "xlog [index]               - Toggle the x-axis log/linear scale of a specified histogram.\n";
	std::cout << prefix_ << "ylog [index]               - Toggle the y-axis log/linear scale of a specified histogram.\n";
	std::cout << prefix_ << "zlog [index]               - Toggle the z-axis log/linear scale of a specified histogram.\n"; 
	std::cout << prefix_ << "xrange [index] [min] [max] - Set the x-axis range of a histogram displayed on the canvas.\n";
	std::cout << prefix_ << "yrange [index] [min] [max] - Set the y-axis range of a histogram displayed on the canvas.\n";
	std::cout << prefix_ << "range [index] [xmin] [xmax] [ymin] [ymax] - Set the range of the x and y axes.\n";
}

/**
 * \param[in] args_
 * \param[out] filename
 */
bool Scanner::SetArgs(std::deque<std::string> &args_, std::string &filename){
	std::string current_arg;
	while(!args_.empty()){
		current_arg = args_.front();
		args_.pop_front();
		
		if(current_arg == "--force-overwrite"){
			std::cout << "Scanner: Forcing overwrite of output file.\n";
			force_overwrite = true;
		}
		else if(current_arg == "--raw-event-mode"){
			std::cout << "Scanner: Using raw event output mode.\n";
			raw_event_mode = true;
		}
		else if(current_arg == "--no-fitting"){
			std::cout << "Scanner: Toggling root fitting OFF.\n";
			use_root_fitting = false;
		}
		else{ filename = current_arg; }
	}
	
	return true;
}

/** Search for an input command and perform the desired action.
  * 
  * \return True if the command is valid and false otherwise.
  */
bool Scanner::CommandControl(std::string cmd_, const std::vector<std::string> &args_){
	if(cmd_ == "refresh"){
		online->Refresh();
	}
	else if(cmd_ == "list"){
		online->PrintHists();
	}
	else if(cmd_ == "set"){
		if(args_.size() == 2){
			int index1 = atoi(args_.at(0).c_str());
			int index2 = atoi(args_.at(1).c_str());
			if(online->ChangeHist(index1, args_.at(1))){ std::cout << message_head << "Set TPad " << index1 << " to histogram '" << args_.at(1) << "'.\n"; }
			else if(online->ChangeHist(index1, index2)){ std::cout << message_head << "Set TPad " << index1 << " to histogram ID " << index2 << ".\n"; }
			else{ std::cout << message_head << "Failed to set TPad " << index1 << " to histogram '" << args_.at(1) << "'!\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'set'\n";
			std::cout << message_head << " -SYNTAX- set [index] [hist]\n";
		}
	}
	else if(cmd_ == "xlog"){
		if(args_.size() == 1){
			int index = atoi(args_.at(0).c_str());
			if(online->ToggleLogX(index)){ std::cout << message_head << "Successfully toggled x-axis log scale for TPad " << index << ".\n"; }
			else{ std::cout << message_head << "Failed to toggle x-axis log scale for TPad " << index << ".\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'xlog'\n";
			std::cout << message_head << " -SYNTAX- xlog [index]\n";
		}
	}
	else if(cmd_ == "ylog"){
		if(args_.size() == 1){
			int index = atoi(args_.at(0).c_str());
			if(online->ToggleLogY(index)){ std::cout << message_head << "Successfully toggled y-axis log scale for TPad " << index << ".\n"; }
			else{ std::cout << message_head << "Failed to toggle y-axis log scale for TPad " << index << ".\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'ylog'\n";
			std::cout << message_head << " -SYNTAX- ylog [index]\n";
		}
	}
	else if(cmd_ == "zlog"){
		if(args_.size() == 1){
			int index = atoi(args_.at(0).c_str());
			if(online->ToggleLogZ(index)){ std::cout << message_head << "Successfully toggled z-axis log scale for TPad " << index << ".\n"; }
			else{ std::cout << message_head << "Failed to toggle z-axis log scale for TPad " << index << ".\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'zlog'\n";
			std::cout << message_head << " -SYNTAX- zlog [index]\n";
		}
	}
	else if(cmd_ == "xrange"){
		if(args_.size() == 3){
			int index = atoi(args_.at(0).c_str());
			float min = atof(args_.at(1).c_str());
			float max = atof(args_.at(2).c_str());
			if(max > min){ 
				if(online->SetXrange(index, min, max)){ std::cout << message_head << "Successfully set range of TPad " << index << ".\n"; }
				else{ std::cout << message_head << "Failed to set range of TPad " << index << "!\n"; }
			}
			else{ std::cout << message_head << "Invalid range for x-axis [" << min << ", " << max << "]\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'xrange'\n";
			std::cout << message_head << " -SYNTAX- xrange [index] [min] [max]\n";
		}
	}
	else if(cmd_ == "yrange"){
		if(args_.size() == 3){
			int index = atoi(args_.at(0).c_str());
			float min = atof(args_.at(1).c_str());
			float max = atof(args_.at(2).c_str());
			if(max > min){ 
				if(online->SetYrange(index, min, max)){ std::cout << message_head << "Successfully set range of TPad " << index << ".\n"; }
				else{ std::cout << message_head << "Failed to set range of TPad " << index << "!\n"; }
			}
			else{ std::cout << message_head << "Invalid range for y-axis [" << min << ", " << max << "]\n"; }
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'yrange'\n";
			std::cout << message_head << " -SYNTAX- yrange [index] [min] [max]\n";
		}
	}
	else if(cmd_ == "range"){
		if(args_.size() == 5){
			int index = atoi(args_.at(0).c_str());
			float xmin = atof(args_.at(1).c_str());
			float xmax = atof(args_.at(2).c_str());
			float ymin = atof(args_.at(3).c_str());
			float ymax = atof(args_.at(4).c_str());
			if(xmax > xmin && ymax > ymin){ 
				if(online->SetRange(index, xmin, xmax, ymin, ymax)){ std::cout << message_head << "Successfully set range of TPad " << index << ".\n"; }
				else{ std::cout << message_head << "Failed to set range of TPad " << index << "!\n"; }
			}
			else{ 
				if(xmax <= xmin && ymax <= ymin){
					std::cout << message_head << "Invalid range for x-axis [" << xmin << ", " << xmax;
					std::cout << "] and y-axis [" << ymin << ", " << ymax << "]\n"; 
				}
				else if(xmax <= xmin){ std::cout << message_head << "Invalid range for x-axis [" << xmin << ", " << xmax << "]\n"; }
				else{ std::cout << message_head << "Invalid range for y-axis [" << ymin << ", " << ymax << "]\n"; }
			}
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'range'\n";
			std::cout << message_head << " -SYNTAX- range [index] [xmin] [xmax] [ymin] [ymax]\n";
		}
	}
	else{ return false; }

	return true;
}

/// Print a status message.	
void Scanner::PrintStatus(std::string prefix_){ 
}

int main(int argc, char *argv[]){
	ScanMain scan_main((Unpacker*)(new Scanner()));
	
	scan_main.SetMessageHeader("Scanner: ");

	return scan_main.Execute(argc, argv);
}
