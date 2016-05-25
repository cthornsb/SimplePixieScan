#include <iostream>

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

// Define the name of the program.
#if not defined(PROG_NAME)
#define PROG_NAME "Scanner"
#endif

///////////////////////////////////////////////////////////////////////////////
// class simpleUnpacker
///////////////////////////////////////////////////////////////////////////////

/** Process all events in the event list.
  * \param[in]  addr_ Pointer to a location in memory. 
  * \return Nothing.
  */
void simpleUnpacker::ProcessRawEvent(void *addr_/*=NULL*/){
	if(!addr_){ return; }
	
	simpleScanner *interface = (simpleScanner*)addr_;

	XiaEvent *current_event = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		rawEvent.pop_front(); // Remove this event from the raw event deque.
		
		// Check that this channel event exists.
		if(!current_event){ continue; }

		// Send the event to the scan interface object for processing.
		interface->AddEvent(current_event);
	}
	
	// Finish up with this raw event.
	interface->ProcessEvents();
}

/** Add an event to generic statistics output.
  * \param[in]  event_ Pointer to the current XIA event. 
  * \return Nothing.
  */
void simpleUnpacker::RawStats(XiaEvent *event_){  }

///////////////////////////////////////////////////////////////////////////////
// class simpleScanner
///////////////////////////////////////////////////////////////////////////////

/// Default constructor.
simpleScanner::simpleScanner() : ScanInterface() {
	force_overwrite = false;
	online_mode = false;
	use_root_fitting = false;
	write_traces = false;
	init = false;
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
	online = NULL;
	output_filename = "out.root";
	events_since_last_update = 0;
	events_between_updates = 5000;
	loaded_files = 0;
}

/// Destructor.
simpleScanner::~simpleScanner(){
	if(init){
		std::cout << msgHeader << "Found " << chanCounts->GetHist()->GetEntries() << " total events.\n";

		// If the root file is open, write the tree and histogram.
		if(root_file->IsOpen()){
			// Write all online diagnostic histograms to the output root file.
			std::cout << msgHeader << "Writing " << online->WriteHists(root_file) << " histograms to root file.\n";

			root_file->cd();

			// Write root trees to output file.
			std::cout << msgHeader << "Writing " << raw_tree->GetEntries() << " raw data entries to root file.\n";
			raw_tree->Write();
			
			std::cout << msgHeader << "Writing " << root_tree->GetEntries() << " processed data entries to root file.\n";
			root_tree->Write();
			
			if(write_traces){
				std::cout << msgHeader << "Writing " << trace_tree->GetEntries() << " raw ADC traces to root file.\n";
				trace_tree->Write();
			}
			
			// Write debug histograms.
			chanCounts->GetHist()->Write();
			chanEnergy->GetHist()->Write();
			
			// Close the root file.
			root_file->Close();
		}

		std::cout << msgHeader << "Processed " << loaded_files << " files.\n";
		std::cout << msgHeader << "Found " << handler->GetTotalEvents() << " start events.\n";
		std::cout << msgHeader << "Total data time is " << handler->GetDeltaEventTime() << " s.\n";
	
		delete mapfile;
		delete configfile;
		delete handler;
		delete root_file;
		delete online;
	}

	GetCore()->CloseUnpacker(); // Close the Unpacker object.
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
		else{ return false; }
	}
	else{ return false; }

	return true;
}

/** ExtraArguments is used to send command line arguments to classes derived
  * from ScanInterface. If ScanInterface receives an unrecognized
  * argument from the user, it will pass it on to the derived class.
  * \param[in]  arg_    The argument to interpret.
  * \param[out] others_ The remaining arguments following arg_.
  * \param[out] ifname  The input filename to send back to use for reading.
  * \return True if the argument was recognized and false otherwise.
  */
bool simpleScanner::ExtraArguments(const std::string &arg_, std::deque<std::string> &others_, std::string &ifname){
	static int count = 0;
	if(arg_ == "--force-overwrite"){
		std::cout << msgHeader << "Forcing overwrite of output file.\n";
		force_overwrite = true;
	}
	else if(arg_ == "--online-mode"){
		std::cout << msgHeader << "Using online mode.\n";
		online_mode = true;
	}
	else if(arg_ == "--fitting"){
		std::cout << msgHeader << "Toggling root fitting ON.\n";
		use_root_fitting = true;
	}
	else if(arg_ == "--traces"){
		std::cout << msgHeader << "Toggling ADC trace output ON.\n";
		write_traces = true;
	}
	else{ // Not a valid option. Must be a filename.
		if(count == 0){ ifname = arg_; } // Set the input filename.
		else if(count == 1){ output_filename = arg_; } // Set the output filename prefix.
		count++;
	}
	
	return true;
}

/** CmdHelp is used to allow a derived class to print a help statement about
  * its own commands. This method is called whenever the user enters 'help'
  * or 'h' into the interactive terminal (if available).
  * \param[in]  prefix_ String to append at the start of any output.
  * \return Nothing.
  */
void simpleScanner::CmdHelp(){
	if(online_mode){
		std::cout << "   refresh <update>           - Set refresh frequency of online diagnostic plots (default=5000).\n";
		std::cout << "   list                       - List all plottable online histograms.\n";
		std::cout << "   set <index> <hist>         - Set the histogram to draw to part of the canvas.\n";
		std::cout << "   xlog <index>               - Toggle the x-axis log/linear scale of a specified histogram.\n";
		std::cout << "   ylog <index>               - Toggle the y-axis log/linear scale of a specified histogram.\n";
		std::cout << "   zlog <index>               - Toggle the z-axis log/linear scale of a specified histogram.\n"; 
		std::cout << "   xrange <index> <min> <max> - Set the x-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   yrange <index> <min> <max> - Set the y-axis range of a histogram displayed on the canvas.\n";
		std::cout << "   unzoom <index> [axis]      - Unzoom the x-axis, the y-axis, or both.\n";
		std::cout << "   range <index> <xmin> <xmax> <ymin> <ymax> - Set the range of the x and y axes.\n";
	}
}

/** ArgHelp is used to allow a derived class to print a help statment about
  * its own command line arguments. This method is called at the end of
  * the ScanInterface::help method.
  * \return Nothing.
  */
void simpleScanner::ArgHelp(){
	std::cout << "   --force-overwrite - Force an overwrite of the output root file if it exists (default=false)\n";
	std::cout << "   --online-mode     - Plot online root histograms for monitoring data (default=false)\n";
	std::cout << "   --fitting         - Use root fitting for high resolution timing (default=false)\n";
	std::cout << "   --traces          - Dump raw ADC traces to output root file (default=false)\n";
}

/** SyntaxStr is used to print a linux style usage message to the screen.
  * \param[in]  name_ The name of the program.
  * \return Nothing.
  */
void simpleScanner::SyntaxStr(char *name_){ 
	std::cout << " usage: " << std::string(name_) << " [input] [options] [output]\n"; 
}

/** Initialize the map file, the config file, the processor handler, 
  * and add all of the required processors.
  * \param[in]  prefix_ String to append to the beginning of system output.
  * \return True upon successfully initializing and false otherwise.
  */
bool simpleScanner::Initialize(std::string prefix_){
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

/** Peform any last minute initialization before processing data. 
  * /return Nothing.
  */
void simpleScanner::FinalInitialization(){
	// Add file header information to the output root file.
	root_file->mkdir("head");
	
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
		bool first_good_channel = true;
		for(int j = 0; j < num_chan; j++){
			entryptr = mapfile->GetMapEntry(i, j);
			if(entryptr->type == "ignore"){ continue; }
			if(first_good_channel){
				root_file->mkdir(dir_names[i].c_str());
				root_file->cd(dir_names[i].c_str());
				first_good_channel = false;			
			}
			TNamed named(chan_names[j].c_str(), (entryptr->type+":"+entryptr->subtype+":"+entryptr->tag).c_str());
			named.Write();
		}
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
			std::stringstream stream;
			if(loaded_files < 10){ stream << "head/file0" << loaded_files; }
			else{ stream << "head/file" << loaded_files; }
			root_file->mkdir(stream.str().c_str());
			root_file->cd(stream.str().c_str());
			std::string name, value;
			for(size_t index = 0; index < finfo->size(); index++){
				finfo->at(index, name, value);
				TNamed named(name.c_str(), value.c_str());
				named.Write();
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
  * This method should only be called from simpleUnpacker::ProcessRawEvent().
  * \param[in]  event_ The raw XiaEvent to add to the channel event deque.
  * \return Nothing.
  */
void simpleScanner::AddEvent(XiaEvent *event_){
	if(!event_){ return; }

	// Link the channel event to its corresponding map entry.
	ChannelEventPair *pair_ = new ChannelEventPair(event_, new ChannelEvent(event_), mapfile->GetMapEntry(event_));

	// Fill the output histograms.
	chanCounts->Fill(event_->chanNum, event_->modNum);
	chanEnergy->Fill(event_->energy, event_->modNum*16+event_->chanNum);

	// Raw event information. Dump raw event information to root file.
	raw_event_module = event_->modNum;
	raw_event_channel = event_->chanNum;
	raw_event_energy = event_->energy;
	raw_event_time = event_->time*8E-9;
	raw_tree->Fill();
	
	// Pass this event to the correct processor
	if(pair_->entry->type == "ignore" || !handler->AddEvent(pair_)){ // Invalid detector type. Delete it
		delete pair_;
		return;
	}

	// This channel is a start signal. Due to the way ScanList
	// packs the raw event, there may be more than one start signal
	// per raw event.
	if(pair_->entry->tag == "start"){ 
		handler->AddStart(pair_);
	}
}

/** Process all channel events read in from the rawEvent.
  * This method should only be called from simpleUnpacker::ProcessRawEvent().
  * \return Nothing.
  */
void simpleScanner::ProcessEvents(){
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
		}
		else{ events_since_last_update++; }
	}
}

int main(int argc, char *argv[]){
	// Define a new unpacker object.
	ScanInterface *scanner = (ScanInterface*)(new simpleScanner());
	
	// Set the output message prefix.
	scanner->SetProgramName(std::string(PROG_NAME));	
	
	// Initialize the scanner.
	scanner->Setup(argc, argv);

	// Run the main loop.
	int retval = scanner->Execute();
	
	// Cleanup.
	delete scanner;

	return retval;
}
