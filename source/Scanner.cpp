#include <iostream>

// PixieCore libraries
#include "Unpacker.hpp"
#include "ScanMain.hpp"
#include "ChannelEvent.hpp"

// Local files
#include "Scanner.hpp"
#include "MapFile.hpp"
#include "ConfigFile.hpp"
#include "ProcessorHandler.hpp"

// Root libraries
#include "TFile.h"
#include "TTree.h"

void Scanner::ProcessRawEvent(){
	ChannelEvent *current_event = NULL;
	MapEntry *current_entry = NULL;
	
	// The first signal in the deque is the start signal for this event
	ChannelEvent *start_event = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		rawEvent.pop_front(); // Remove this event from the raw event deque.
		
		if(!current_event){ continue; }
		
		current_entry = mapfile->GetMapEntry(current_event);
		
		if(!current_entry){ continue; }

		if(!raw_event_mode){ // Standard operation. Individual processors will handle output
			// Pass this event to the correct processor
			if(current_entry->type == "ignore" || !handler->AddEvent(current_event, current_entry)){ // Invalid detector type. Delete it
				delete current_event;
			}
		
			// This channel is a start signal. Due to the way ScanList
			// packs the raw event, there may be more than one start signal
			// per raw event.
			if(current_entry->tag == "start"){ 
				if(start_event != NULL && debug_mode){ std::cout << "ProcessRawEvent: Found more than one start event in rawEvent!\n"; }
				start_event = current_event;
			}
		}
		else{ // Raw event mode operation. Dump raw event information to root file.
			structure.Append(current_event->modNum, current_event->chanNum, current_event->time, current_event->energy);
			delete current_event;
		}
	}
	
	if(!raw_event_mode){
		// Call each processor to do the processing. Each
		// processor will remove the channel events when finished.
		if(start_event && handler->Process(start_event)){
			// This event had at least one valid signal
			root_tree->Fill();
		}
		
		// Zero all of the processors.
		handler->ZeroAll();
	}
	else{
		root_tree->Fill();
		structure.Zero();
	}
}

Scanner::Scanner(){
	force_overwrite = false;
	raw_event_mode = false;
	use_root_fitting = true;
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
	output_filename = "out.root";
}

Scanner::~Scanner(){
	delete mapfile;
	delete configfile;
	
	if(init){
		std::cout << "Unpacker: Found " << handler->GetTotalEvents() << " start events.\n";
		std::cout << "Unpacker: Total data time is " << handler->GetDeltaEventTime() << " s.\n";
	}
	
	delete handler;

	if(root_file){
		if(root_file->IsOpen()){
			std::cout << "Unpacker: Writing " << root_tree->GetEntries() << " entries to root file.\n";
			root_file->cd();
			root_tree->Write();
			root_file->Close();
		}
		delete root_file;
	}
	
	Close(); // Close the Unpacker object.
}

/// Initialize the map file, the config file, the processor handler, and add all of the required processors.
bool Scanner::Initialize(std::string prefix_){
	if(init){ return false; }

	std::cout << prefix_ << "Reading map file ./setup/map.dat\n";
	mapfile = new MapFile("./setup/map.dat");
	std::cout << prefix_ << "Reading config file ./setup/config.dat\n";
	configfile = new ConfigFile("./setup/config.dat");
	event_width = configfile->event_width * 125; // = event_width * 1E-6(s/us) / 8E-9(s/clock)
	std::cout << prefix_ << "Setting event width to " << configfile->event_width << " μs (" << event_width << " pixie clock ticks).\n";
	handler = new ProcessorHandler();

	std::vector<MapEntry> *types = mapfile->GetTypes();
	for(std::vector<MapEntry>::iterator iter = types->begin(); iter != types->end(); iter++){
		if(iter->type == "ignore" || !handler->CheckProcessor(iter->type)){ continue; }
		else if(handler->AddProcessor(iter->type, mapfile)){ std::cout << prefix_ << "Added " << iter->type << " processor to the processor list.\n"; }
		else{ std::cout << prefix_ << "Failed to add " << iter->type << " processor to the processor list!\n"; }
	}
	
	std::cout << prefix_ << "Initializing root output.\n";

	if(force_overwrite){ root_file = new TFile(output_filename.c_str(), "RECREATE"); }
	else{ root_file = new TFile(output_filename.c_str(), "CREATE"); }

	if(!root_file->IsOpen()){
		std::cout << prefix_ << "Failed to open output root file '" << output_filename << "'!\n";
		root_file->Close();
		delete root_file;
		root_file = NULL;
		return false;
	}

	root_tree = new TTree("Pixie16", "Pixie16 data");

	if(!raw_event_mode){ // Standard operation
		handler->InitRootOutput(root_tree);
	}
	else{ // Raw event output only. Processors will not be called!
		root_tree->Branch("RawEvent", &structure);
	}
	
	if(!use_root_fitting){ handler->ToggleFitting(); }

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
	std::cout << prefix_ << "--force-overwrite | Force an overwrite of the output root file if it exists (default=false)\n";
	std::cout << prefix_ << "--raw-event-mode  | Write raw channel information to the output root file (default=false)\n";
	std::cout << prefix_ << "--no-fitting      | Do not use root fitting for high resolution timing (default=true)\n";
}

/** 
 *	\param[in] prefix_ 
 */
void Scanner::CmdHelp(std::string prefix_){
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
			/*if(args_.empty()){
				std::cout << " Error: Missing required argument to option '--mod'!\n";
				return false;
			}
			mod_ = atoi(args_.front().c_str());*/
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
	/*if(cmd_ == "set"){ // Toggle debug mode
		if(args_.size() == 2){
			mod_ = atoi(args_.at(0).c_str());
			chan_ = atoi(args_.at(1).c_str());
			resetGraph_ = true;
		}
		else{
			std::cout << message_head << "Invalid number of parameters to 'set'\n";
			std::cout << message_head << " -SYNTAX- set [module] [channel]\n";
		}
	}*/
	//else{ return false; }

	return false;
}

/// Print a status message.	
void Scanner::PrintStatus(std::string prefix_){ 
	//std::cout << prefix_ << "Found " << num_traces << " traces and displayed " << num_displayed << ".\n"; 
}

int main(int argc, char *argv[]){
	ScanMain scan_main((Unpacker*)(new Scanner()));
	
	scan_main.SetMessageHeader("Scanner: ");

	return scan_main.Execute(argc, argv);
}
