#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <time.h>
#include <algorithm>

// PixieCore libraries
#include "Unpacker.hpp"
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
	
	// The first signal in the deque is the start signal for this event
	ChannelEvent *start_event = NULL;
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		
		if(!raw_event_mode){ // Standard operation. Individual processors will handle output
			// Pass this event to the correct processor
			if(current_event->entry->type == "ignore" || !handler->AddEvent(current_event)){ // Invalid detector type. Delete it
				delete current_event;
			}
		
			// This channel is a start signal. Due to the way ScanList
			// packs the raw event, there may be more than one start signal
			// per raw event.
			if(current_event->entry->tag == "start" || current_event->entry->tag == "left"){ 
				if(start_event != NULL/* && debug_mode*/){ std::cout << "ProcessRawEvent: Found more than one start event in rawEvent!\n"; }
				start_event = current_event;
			}
		}
		else{ // Raw event mode operation. Dump raw event information to root file.
			structure.Append(current_event->modNum, current_event->chanNum, current_event->time, current_event->energy);
			delete current_event;
		}
		
		// Remove this event from the raw event deque
		rawEvent.pop_front();
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

bool Scanner::Initialize(){
	if(init){ return false; }

	mapfile = new MapFile("./setup/map.dat");
	configfile = new ConfigFile("./setup/config.dat");
	handler = new ProcessorHandler();
	
	std::vector<MapEntry> *types = mapfile->GetTypes();
	for(std::vector<MapEntry>::iterator iter = types->begin(); iter != types->end(); iter++){
		if(iter->type == "ignore"){ continue; }
		else if(handler->AddProcessor(iter->type)){ std::cout << "Unpacker: Added " << iter->type << " processor to the processor list.\n"; }
		else{ std::cout << "Unpacker: Failed to add " << iter->type << " processor to the processor list!\n"; }
	}
	
	return (init = true);
}

Scanner::Scanner(){
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;
}

Scanner::Scanner(std::string fname_, bool overwrite_/*=true*/, bool debug_mode_/*=false*/){
	Initialize();
	InitRootOutput(fname_, overwrite_);
}

Scanner::~Scanner(){
	if(init){
		delete mapfile;
		delete configfile;
		
		std::cout << "Unpacker: Found " << handler->GetTotalEvents() << " start events.\n";
		std::cout << "Unpacker: Total data time is " << handler->GetDeltaEventTime() << " s.\n";
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
	}
}

bool Scanner::InitRootOutput(std::string fname_, bool overwrite_/*=true*/){
	if(!init){ Initialize(); }
	
	if(root_file && root_file->IsOpen()){ return false; }
	
	std::cout << "Unpacker: Initializing root output.\n";
	
	if(overwrite_){ root_file = new TFile(fname_.c_str(), "RECREATE"); }
	else{ root_file = new TFile(fname_.c_str(), "CREATE"); }
	
	if(!root_file->IsOpen()){
		std::cout << "Unpacker: Failed to open output root file '" << fname_ << "'!\n";
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
	
	return true;
}

bool Scanner::SetHiResMode(bool state_/*=true*/){
	if(!handler){ return false; }

	handler->SetHiResMode(state_);
	
	return true;
}
