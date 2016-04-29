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
#include "TH1.h"
#include "TNamed.h"

/// Process all events in the event list.
void Scanner::ProcessRawEvent(){
	PixieEvent *current_event = NULL;
	ChannelEvent *channel_event = NULL;
	ChannelEventPair *current_pair = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		rawEvent.pop_front(); // Remove this event from the raw event deque.
		
		// Check that this channel event exists.
		if(!current_event){ continue; }

		channel_event = new ChannelEvent(current_event);

		// Fill the output histograms.
		chanCounts->Fill(current_event->chanNum, current_event->modNum);
		chanEnergy->Fill(current_event->energy, current_event->modNum*16+current_event->chanNum);

		// Link the channel event to its corresponding map entry.
		current_pair = new ChannelEventPair(current_event, channel_event, mapfile->GetMapEntry(current_event));

		// Raw event information. Dump raw event information to root file.
		raw_event_module = current_event->modNum;
		raw_event_channel = current_event->chanNum;
		raw_event_energy = current_event->energy;
		raw_event_time = current_event->time*8E-9;
		raw_tree->Fill();
	
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
	
	// Call each processor to do the processing. Each
	// processor will remove the channel events when finished.
	if(handler->Process()){ // This event had at least one valid signal
		// Fill the root tree with processed data.
		root_tree->Fill();

		// Fill the ADC trace tree with raw traces.		
		if(write_traces){ trace_tree->Fill(); }
	}
	
	// Zero all of the processors.
	handler->ZeroAll();
	
	// Check for the need to update the online canvas.
	if(online_mode){
		if(events_since_last_update >= events_between_updates){
			online->Refresh();
			events_since_last_update = 0;
			std::cout << "refresh!\n";
		}
		else{ events_since_last_update++; }
	}
}

Scanner::Scanner(){
	force_overwrite = false;
	online_mode = false;
	use_root_fitting = false;
	write_traces = false;
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
	online = NULL;
	output_filename = "out.root";
	events_since_last_update = 0;
	events_between_updates = 5000;
}

Scanner::~Scanner(){
	if(init){
		std::cout << "Scanner: Found " << chanCounts->GetHist()->GetEntries() << " total events.\n";

		// If the root file is open, write the tree and histogram.
		if(root_file->IsOpen()){
			// Write all online diagnostic histograms to the output root file.
			std::cout << "Scanner: Writing " << online->WriteHists(root_file) << " histograms to root file.\n";

			root_file->cd();

			// Write root trees to output file.
			std::cout << "Scanner: Writing " << raw_tree->GetEntries() << " raw data entries to root file.\n";
			raw_tree->Write();
			
			std::cout << "Scanner: Writing " << root_tree->GetEntries() << " processed data entries to root file.\n";
			root_tree->Write();
			
			if(write_traces){
				std::cout << "Scanner: Writing " << trace_tree->GetEntries() << " raw ADC traces to root file.\n";
				trace_tree->Write();
			}
			
			// Write debug histograms.
			chanCounts->GetHist()->Write();
			chanEnergy->GetHist()->Write();
			
			// Close the root file.
			root_file->Close();
		}

		std::cout << "Scanner: Found " << handler->GetTotalEvents() << " start events.\n";
		std::cout << "Scanner: Total data time is " << handler->GetDeltaEventTime() << " s.\n";
	
		delete mapfile;
		delete configfile;
		delete handler;
		delete root_file;
		delete online;
	}

	Close(); // Close the Unpacker object.
}

/// Initialize the map file, the config file, the processor handler, and add all of the required processors.
bool Scanner::Initialize(std::string prefix_){
	if(init){ return false; }

	// Setup a 2d histogram for tracking all channel counts.
	chanCounts = new Plotter("chanCounts", "Recorded Counts for Module vs. Channel", "COLZ", "Channel", 16, 0, 16, "Module", 14, 0, 14);

	// Setup a 2d histogram for tracking all channel counts.
	chanEnergy = new Plotter("chanEnergy", "Channel vs. Energy", "COLZ", "Energy (a.u.)", 32768, 0, 32768, "Channel", 224, 0, 224);

	// Initialize the online data processor.
	online = new OnlineProcessor();

	if(online_mode){
		online->SetDisplayMode();
	
		// Add the raw histograms to the online processor.
		online->AddHist(chanCounts);
		online->AddHist(chanEnergy);
	
		// Set the first and second histograms to channel count histogram and energy histogram.
		online->ChangeHist(0, 0);
		online->ChangeHist(1, 1);
		online->Refresh();
	}

	// Initialize map file, config file, and processor handler.
	std::cout << prefix_ << "Reading map file ./setup/map.dat\n";
	mapfile = new MapFile("./setup/map.dat");
	if(!mapfile->IsInit()){ // Failed to read map file.
		std::cout << prefix_ << "Failed to read map file './setup/map.dat'.\n";
		delete mapfile;
		return false;
	}
	std::cout << prefix_ << "Reading config file ./setup/config.dat\n";
	configfile = new ConfigFile("./setup/config.dat");
	if(!configfile->IsInit()){ // Failed to read config file.
		std::cout << prefix_ << "Failed to read configuration file './setup/configp.dat'.\n";
		delete mapfile;
		delete configfile;
		return false;
	}
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
	root_tree = new TTree("data", "Pixie data");
	
	// Setup the raw data tree for output.
	raw_tree = new TTree("raw", "Raw pixie data");
	
	// Add branches to the raw data tree.
	raw_tree->Branch("mod", &raw_event_module);
	raw_tree->Branch("chan", &raw_event_channel);
	raw_tree->Branch("energy", &raw_event_energy);
	raw_tree->Branch("time", &raw_event_time);

	// Add branches to the output tree.
	handler->InitRootOutput(root_tree);

	// Set processor options.
	if(use_root_fitting){ handler->ToggleFitting(); }
	if(write_traces){ 
		trace_tree = new TTree("trace", "Raw pixie ADC traces");
		handler->InitTraceOutput(trace_tree); 
	}

	return (init = true);
}

/// Perform last minute procedures before running.
void Scanner::FinalInitialization(){
	if(!scan_main){ return; }
	fileInformation *finfo = scan_main->GetFileInfo();
	if(!finfo){ return; }

	// Add file header information to the output root file.
	std::string name, value;
	root_file->mkdir("head");
	root_file->cd("head");
	for(size_t index = 0; index < finfo->size(); index++){
		finfo->at(index, name, value);
		std::cout << name << ", " << value << std::endl;
		TNamed named(name.c_str(), value.c_str());
		named.Write();
	}
	
	// Add all map entries to the output root file.
	const int num_mod = mapfile->GetMaxModules();
	const int num_chan = mapfile->GetMaxChannels();
	MapEntry *entryptr;

	std::string dir_names[num_mod];
	std::string chan_names[num_chan];
	for(int i = 0; i < num_mod; i++){
		std::stringstream stream;
		if(i < 10){ stream << "0" << i; }
		else{ stream << i; }
		dir_names[i] = "map/mod" + stream.str();
	}
	for(int i = 0; i < num_chan; i++){
		std::stringstream stream;
		if(i < 10){ stream << "0" << i; }
		else{ stream << i; }
		chan_names[i] = "chan" + stream.str();
	}

	root_file->mkdir("map");
	for(int i = 0; i < num_mod; i++){
		root_file->mkdir(dir_names[i].c_str());
		root_file->cd(dir_names[i].c_str());
		for(int j = 0; j < num_chan; j++){
			entryptr = mapfile->GetMapEntry(i, j);
			if(entryptr->type == "ignore"){ continue; }
			TNamed named(chan_names[j].c_str(), (entryptr->type+":"+entryptr->subtype+":"+entryptr->tag).c_str());
			named.Write();
		}
	}
}

/// Return the syntax string for this program.
void Scanner::SyntaxStr(const char *name_, std::string prefix_){ 
	std::cout << prefix_ << "SYNTAX: " << std::string(name_) << " <input> <options> <output>\n"; 
}
	
/**
 * \param[in] prefix_
 */
void Scanner::ArgHelp(std::string prefix_){
	std::cout << prefix_ << "--force-overwrite - Force an overwrite of the output root file if it exists (default=false)\n";
	std::cout << prefix_ << "--online-mode     - Plot online root histograms for monitoring data (default=false)\n";
	std::cout << prefix_ << "--fitting         - Use root fitting for high resolution timing (default=false)\n";
	std::cout << prefix_ << "--traces          - Dump raw ADC traces to output root file (default=false)\n";
}

/** 
 *	\param[in] prefix_ 
 */
void Scanner::CmdHelp(std::string prefix_){
	if(online_mode){
		std::cout << prefix_ << "refresh <update>           - Set refresh frequency of online diagnostic plots (default=5000).\n";
		std::cout << prefix_ << "list                       - List all plottable online histograms.\n";
		std::cout << prefix_ << "set [index] [hist]         - Set the histogram to draw to part of the canvas.\n";
		std::cout << prefix_ << "xlog [index]               - Toggle the x-axis log/linear scale of a specified histogram.\n";
		std::cout << prefix_ << "ylog [index]               - Toggle the y-axis log/linear scale of a specified histogram.\n";
		std::cout << prefix_ << "zlog [index]               - Toggle the z-axis log/linear scale of a specified histogram.\n"; 
		std::cout << prefix_ << "xrange [index] [min] [max] - Set the x-axis range of a histogram displayed on the canvas.\n";
		std::cout << prefix_ << "yrange [index] [min] [max] - Set the y-axis range of a histogram displayed on the canvas.\n";
		std::cout << prefix_ << "unzoom [index] <axis>      - Unzoom the x-axis, the y-axis, or both.\n";
		std::cout << prefix_ << "range [index] [xmin] [xmax] [ymin] [ymax] - Set the range of the x and y axes.\n";
	}
}

/**
 * \param[in] args_
 * \param[out] filename
 */
bool Scanner::SetArgs(std::deque<std::string> &args_, std::string &filename){
	int count = 0;
	std::string current_arg;
	while(!args_.empty()){
		current_arg = args_.front();
		args_.pop_front();
		
		if(current_arg == "--force-overwrite"){
			std::cout << "Scanner: Forcing overwrite of output file.\n";
			force_overwrite = true;
		}
		else if(current_arg == "--online-mode"){
			std::cout << "Scanner: Using online mode.\n";
			online_mode = true;
		}
		else if(current_arg == "--fitting"){
			std::cout << "Scanner: Toggling root fitting ON.\n";
			use_root_fitting = true;
		}
		else if(current_arg == "--traces"){
			std::cout << "Scanner: Toggling ADC trace output ON.\n";
			write_traces = true;
		}
		else{ // Not a valid option. Must be a filename.
			if(count == 0){ filename = current_arg; } // Set the input filename.
			else if(count == 1){ output_filename = current_arg; } // Set the output filename prefix.
			count++;
		}
	}
	
	return true;
}

/** Search for an input command and perform the desired action.
  * 
  * \return True if the command is valid and false otherwise.
  */
bool Scanner::CommandControl(std::string cmd_, const std::vector<std::string> &args_){
	if(online_mode){
		if(cmd_ == "refresh"){
			if(args_.size() >= 1){
				int frequency = atoi(args_.at(0).c_str());
				if(frequency > 0){ 
					std::cout << message_head << "Set canvas update frequency to " << frequency << " events.\n"; 
					events_between_updates = frequency;
				}
				else{ std::cout << message_head << "Failed to set canvas update frequency to " << frequency << " events!\n"; }
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
			if(args_.size() >= 1){
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
			if(args_.size() >= 1){
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
			if(args_.size() >= 1){
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
			if(args_.size() >= 3){
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
			if(args_.size() >= 3){
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
		else if(cmd_ == "unzoom"){
			if(args_.size() >= 1){
				int index = atoi(args_.at(0).c_str());
				if(args_.size() >= 2){
					if(args_.at(1) == "x" || args_.at(1) == "X" || args_.at(1) == "0"){
						online->ResetXrange(index);
						std::cout << message_head << "Reset range of X axis.\n";
					}
					else if(args_.at(1) == "y" || args_.at(1) == "Y" || args_.at(1) == "1"){
						online->ResetYrange(index);
						std::cout << message_head << "Reset range of Y axis.\n";
					}
					else{ std::cout << message_head << "Unknown axis (" << args_.at(1) << ")!\n"; }
				}
				else{
					online->ResetRange(index);
					std::cout << message_head << "Reset range of X and Y axes.\n";
				}
			}
			else{
				std::cout << message_head << "Invalid number of parameters to 'unzoom'\n";
				std::cout << message_head << " -SYNTAX- unzoom [index] <axis>\n";
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
	}
	else{ return false; }

	return true;
}

/// Print a status message.	
void Scanner::PrintStatus(std::string prefix_){ 
}

int main(int argc, char *argv[]){
	// Define a new unpacker object.
	Unpacker *scanner = (Unpacker*)(new Scanner());
	
	// Setup the ScanMain object and link it to the unpacker object.
	ScanMain scan_main(scanner);
	
	// Initialize the scanner.
	scan_main.Initialize(argc, argv);

	// Link the unpacker object back to the ScanMain object so we may
	// access its command line arguments and options.
	scanner->SetScanMain(&scan_main);
	
	// Set the output message prefix.
	scan_main.SetMessageHeader("Scanner: ");

	// Run the main loop.
	return scan_main.Execute();
}
