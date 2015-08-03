#include <iostream>
#include <sstream>
#include <string.h>
#include <time.h>
#include <algorithm>

#include "Unpacker.hpp"
#include "MapFile.hpp"
#include "ConfigFile.hpp"
#include "ChannelEvent.hpp"
#include "ProcessorHandler.hpp"

#include "TFile.h"
#include "TTree.h"

std::string to_str(const int &input_){
	std::stringstream stream;
	stream << input_;
	return stream.str();
}

bool is_in(const std::vector<std::string> &vec_, const std::string &input_){
	for(std::vector<std::string>::const_iterator iter = vec_.begin(); iter != vec_.end(); iter++){
		if((*iter) == input_){ return true; }
	}
	return false;
}

void Unpacker::ClearRawEvent(){
	while(!rawEvent.empty()){
		delete rawEvent.front();
		rawEvent.pop_front();
	}
}

void Unpacker::ClearEventList(){
	while(!eventList.empty()){
		delete eventList.front();
		eventList.pop_front();
	}
}

void Unpacker::DeleteCurrentEvent(){
	if(eventList.empty()){ return; }
	delete eventList.front();
	eventList.pop_front();
}

void Unpacker::ProcessRawEvent(){
	ChannelEvent *current_event = NULL;
	std::string type, subtype, tag;
	
	// The first signal in the deque is the start signal for this event
	ChannelEvent *start_event = rawEvent.front();
	
	// Fill the processor event deques with events
	while(!rawEvent.empty()){
		current_event = rawEvent.front();
		
		// Check that this detector is valid
		if(current_event->entry){
			// Get detector type information
			current_event->entry->get(type, subtype, tag);
		
			if(!raw_event_mode){ // Standard operation. Individual processors will handle output
				// Pass this event to the correct processor
				if(type == "ignore" || !handler->AddEvent(current_event, type)){ // Invalid detector type. Delete it
					delete current_event;
				}
			
				// This channel is a start signal. Due to the way ScanList
				// packs the raw event, there may only be one start signal
				// per raw event.
				if(tag == "start"){ 
					start_event = current_event;
				}
			}
			else{ // Raw event mode operation. Dump raw event information to root file.
				structure.Append(current_event->modNum, current_event->chanNum, current_event->time, current_event->energy);
				delete current_event;
			}
		}
		
		// Remove this event from the raw event deque
		rawEvent.pop_front();
	}
	
	if(!raw_event_mode){
		// Call each processor to do the processing. Each
		// processor will remove the channel events when finished.
		if(handler->Process(start_event)){
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

void Unpacker::ScanList(){
	//unsigned long chanTime, eventTime;

	// local variables for the times of the current event, previous
	// event and time difference between the two
	double lastTime = 0.0;
	double currTime = 0.0;
	
	int mod, chan;
	std::string type, subtype, tag;
	
	bool found_valid_start = false;

	//loop over the list of channels that fired in this buffer
	ChannelEvent *current_event;
	while(!eventList.empty()){
		current_event = eventList.front();
	
		mod = current_event->modNum;
		chan = current_event->chanNum;
		
		// Set this channel event's pointer to its corresponding map entry
		current_event->entry = mapfile->GetMapEntry(mod, chan);
		if(!current_event->entry){
			std::cout << "ScanList: Encountered non-physical Pixie ID (mod = " << mod << ", chan = " << chan << ")\n";
			DeleteCurrentEvent();
			continue;
		}
		else if(current_event->entry->type == "ignore"){ // Skip this channel
			DeleteCurrentEvent();
			continue;
		}

		// Get detector type information
		current_event->entry->get(type, subtype, tag);
		
		// Search for a start signal
		if(!found_valid_start){
			if(tag != "start"){ 
				DeleteCurrentEvent();
				continue; 
			}
			found_valid_start = true;
			lastTime = current_event->time;
		}
		else if(tag == "start"){ // Encountered a new start signal before the max event width was exceeded
			if(!rawEvent.empty()){ ProcessRawEvent(); }
			lastTime = current_event->time;
		}
		
		// This is a channel we're interested in
		// Retrieve the current event time
		currTime = current_event->time;

		// If the time difference between the current and previous event is 
		// larger than the event width, finalize the current event, otherwise
		// treat this as part of the current event
		/*if((currTime - lastTime) > configfile->event_width && !rawEvent.empty()){
			found_valid_start = false;
			ProcessRawEvent();
		}*/

		/*unsigned long dtimebin = 2000 + eventTime - chanTime;
		if(dtimebin < 0 || dtimebin > 16384){
			std::cout << "ScanList: Strange dtime for mod = " << mod << ", chan = " << chan << "\n";
		}*/
		
		// Push this channel event into the rawEvent.
		rawEvent.push_back(current_event);
		
		// Remove this event from the event list but do not delete it yet.
		// Deleting of the channel events will be handled by clearing the rawEvent.
		eventList.pop_front();
	} //end loop over event list

	// Process the last event in the buffer
	if(rawEvent.size() > 0){
		ProcessRawEvent();
	}
	
	// Check that the event list is empty. It should be...
	if(!eventList.empty()){
		std::cout << "ScanList: Attempting to exit ScanList with elements in the event list!\n";
		ClearEventList();
	}
}	

void Unpacker::SortList(){
	sort(eventList.begin(), eventList.end(), &ChannelEvent::CompareTime);
}

int Unpacker::ReadBuffer(unsigned int *buf, unsigned long *bufLen){						
	// multiplier for high bits of 48-bit time
	static const double HIGH_MULT = pow(2., 32.); 

	unsigned int modNum;
	unsigned long numEvents = 0;
	unsigned int *bufStart = buf;

	// Determine the number of words in the buffer
	*bufLen = *buf++;

	// Read the module number
	modNum = *buf++;

	ChannelEvent *lastVirtualChannel = NULL;

	if(*bufLen > 0){ // Check if the buffer has data
		if(*bufLen == 2){ // this is an empty channel
			return 0;
		}
		while( buf < bufStart + *bufLen ){
			ChannelEvent *currentEvt = new ChannelEvent();

			// decoding event data... see pixie16app.c
			// buf points to the start of channel data
			unsigned int chanNum      = (buf[0] & 0x0000000F);
			unsigned int slotNum      = (buf[0] & 0x000000F0) >> 4;
			unsigned int crateNum     = (buf[0] & 0x00000F00) >> 8;
			unsigned int headerLength = (buf[0] & 0x0001F000) >> 12;
			unsigned int eventLength  = (buf[0] & 0x1FFE0000) >> 17;
			
			currentEvt->virtualChannel = ((buf[0] & 0x20000000) != 0);
			currentEvt->saturatedBit   = ((buf[0] & 0x40000000) != 0);
			currentEvt->pileupBit      = ((buf[0] & 0x80000000) != 0);

			// Rev. D header lengths not clearly defined in pixie16app_defs
			//! magic numbers here for now
			if(headerLength == 1){
				// this is a manual statistics block inserted by the poll program
				/*stats.DoStatisticsBlock(&buf[1], modNum);
				buf += eventLength;
				numEvents = -10;*/
				continue;
			}
			if(headerLength != 4 && headerLength != 8 && headerLength != 12 && headerLength != 16){
				std::cout << "ReadBuffer: Unexpected header length: " << headerLength << std::endl;
				std::cout << "ReadBuffer:   Buffer " << modNum << " of length " << *bufLen << std::endl;
				std::cout << "ReadBuffer:   CHAN:SLOT:CRATE " << chanNum << ":" << slotNum << ":" << crateNum << std::endl;
				// advance to next event and continue
				// buf += EventLength;
				// continue;

				// skip the rest of this buffer
				return numEvents;
			}

			unsigned int lowTime     = buf[1];
			unsigned int highTime    = buf[2] & 0x0000FFFF;
			unsigned int cfdTime     = (buf[2] & 0xFFFF0000) >> 16;
			unsigned int energy      = buf[3] & 0x0000FFFF;
			unsigned int traceLength = (buf[3] & 0xFFFF0000) >> 16;

			if(headerLength == 8 || headerLength == 16){
				// Skip the onboard partial sums for now 
				// trailing, leading, gap, baseline
			}

			if(headerLength >= 12){
				int offset = headerLength - 8;
				for (int i=0; i < currentEvt->numQdcs; i++){
					currentEvt->qdcValue[i] = buf[offset + i];
				}
			}	 

			// One last check
			if( traceLength / 2 + headerLength != eventLength ){
				std::cout << "ReadBuffer: Bad event length (" << eventLength << ") does not correspond with length of header (";
				std::cout << headerLength << ") and length of trace (" << traceLength << ")" << std::endl;
				buf += eventLength;
				continue;
			}

			// Handle multiple crates
			modNum += 100 * crateNum;

			currentEvt->chanNum = chanNum;
			currentEvt->modNum = modNum;
			/*if(currentEvt->virtualChannel){
				DetectorLibrary* modChan = DetectorLibrary::get();

				currentEvt->modNum += modChan->GetPhysicalModules();
				if(modChan->at(modNum, chanNum).HasTag("construct_trace")){
					lastVirtualChannel = currentEvt;
				}
			}*/

			currentEvt->energy = energy;
			if(currentEvt->saturatedBit){ currentEvt->energy = 16383; }
					
			currentEvt->trigTime = lowTime;
			currentEvt->cfdTime	= cfdTime;
			currentEvt->eventTimeHi = highTime;
			currentEvt->eventTimeLo = lowTime;
			currentEvt->time = highTime * HIGH_MULT + lowTime;

			buf += headerLength;
			// Check if trace data follows the channel header
			if( traceLength > 0 ){
				// sbuf points to the beginning of trace data
				unsigned short *sbuf = (unsigned short *)buf;

				currentEvt->trace.reserve(traceLength);

				/*if(currentEvt->saturatedBit)
					currentEvt->trace.SetValue("saturation", 1);*/

				if( lastVirtualChannel != NULL && lastVirtualChannel->trace.empty() ){		
					lastVirtualChannel->trace.assign(traceLength, 0);
				}
				// Read the trace data (2-bytes per sample, i.e. 2 samples per word)
				for(unsigned int k = 0; k < traceLength; k ++){		
					currentEvt->trace.push_back(sbuf[k]);

					if(lastVirtualChannel != NULL){
						lastVirtualChannel->trace[k] += sbuf[k];
					}
				}
				buf += traceLength / 2;
			}
 
			eventList.push_back(currentEvt);

			numEvents++;
		}
	} 
	else{ // if buffer has data
		std::cout << "ReadBuffer: ERROR IN ReadBuffData, LIST UNKNOWN" << std::endl;
		return -100;
	}
	
	return numEvents;
}

Unpacker::Unpacker(){
	mapfile = NULL;
	configfile = NULL;
	handler = NULL;

	raw_event_mode = false;
	debug_mode = false;
	full_event = false;
	init = false;
	
	root_file = NULL;
	root_tree = NULL;
}

Unpacker::Unpacker(std::string fname_, bool overwrite_/*=true*/, bool debug_mode_/*=false*/){
	raw_event_mode = false;
	Initialize();
	InitRootOutput(fname_, overwrite_);
}

Unpacker::~Unpacker(){
	if(init){
		delete mapfile;
		delete configfile;
		delete handler;
	
		ClearRawEvent();
		ClearEventList();
	
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

bool Unpacker::Initialize(){
	if(init){ return false; }

	mapfile = new MapFile("./setup/map.dat");
	configfile = new ConfigFile("./setup/config.dat");
	handler = new ProcessorHandler();
	
	std::vector<DetType> *types = mapfile->GetTypes();
	for(std::vector<DetType>::iterator iter = types->begin(); iter != types->end(); iter++){
		if(iter->type == "ignore"){ continue; }
		else if(handler->AddProcessor(iter->type)){ std::cout << "Unpacker: Added " << iter->type << " processor to the processor list.\n"; }
		else{ std::cout << "Unpacker: Failed to add " << iter->type << " processor to the processor list!\n"; }
	}
	
	debug_mode = false;
	full_event = false;
	
	root_file = NULL;
	root_tree = NULL;
	
	return (init = true);
}

bool Unpacker::InitRootOutput(std::string fname_, bool overwrite_/*=true*/){
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

bool Unpacker::ReadSpill(char *ibuf, unsigned int nWords, bool is_verbose/*=true*/){
	if(!init || !root_tree){ return false; }

	const unsigned int maxVsn = 14; // No more than 14 pixie modules per crate
	unsigned int nWords_read = 0;
	
	//static clock_t clockBegin; // Initialization time
	//time_t tmsBegin;

	std::vector<ChannelEvent*> eventList; // Vector to hold the events

	// Local version of ibuf pointer
	unsigned int *data = (unsigned int *)ibuf;

	int retval = 0; // return value from various functions
	
	unsigned long bufLen;
	
	// Various event counters 
	unsigned long numEvents = 0;
	static int counter = 0; // the number of times this function is called
	static int evCount;	 // the number of times data is passed to ScanList
	static unsigned int lastVsn; // the last vsn read from the data
	time_t theTime = 0;

	// Initialize the scan program before the first event 
	if(counter==0){
		// Retrieve the current time for use later to determine the total
		// running time of the analysis.
		//clockBegin = time(&tmsBegin);

		// After completion the descriptions of all channels are in the mapfile
		// vector, the DetectorDriver and rawEventent have been initialized with the
		// detectors that will be used in this analysis.
		//std::cout << "Using event width " << pixie::eventInSeconds * 1e6 << " us (" << pixie::eventWidth << " in pixie16 clock tics).\n\n";

		std::cout << "ReadSpill: Using detector types...\n";
		mapfile->PrintAllTypes();

		lastVsn=-1; // Set last vsn to -1 so we expect vsn 0 first 	
	}
	counter++;
 
	unsigned int lenRec = 0xFFFFFFFF;
	unsigned int vsn = 0xFFFFFFFF;
	bool fullSpill=false; // True if spill had all vsn's

	// While the current location in the buffer has not gone beyond the end
	// of the buffer (ignoring the last three delimiters, continue reading
	while (nWords_read <= nWords){
		// Retrieve the record length and the vsn number
		lenRec = data[nWords_read]; // Number of words in this record
		vsn = data[nWords_read+1]; // Module number
	
		// Check sanity of record length and vsn
		if(lenRec > maxWords || (vsn > maxVsn && vsn != 9999 && vsn != 1000)){ 
			if(is_verbose){
				std::cout << "ReadSpill: SANITY CHECK FAILED: lenRec = " << lenRec << ", vsn = " << vsn << ", read " << nWords_read << " of " << nWords << std::endl;
			}
			return false;	
		}

		// If the record length is 6, this is an empty channel.
		// Skip this vsn and continue with the next
		//! Revision specific, so move to ReadBuffData
		if(lenRec==6){
			nWords_read += lenRec;
			lastVsn=vsn;
			continue;
		}

		// If both the current vsn inspected is within an acceptable 
		// range, begin reading the buffer.
		if(vsn < maxVsn){
			if(lastVsn != 0xFFFFFFFF && vsn != lastVsn+1){
				if(is_verbose){ 
					std::cout << "ReadSpill: MISSING BUFFER " << lastVsn+1 << ", lastVsn = " << lastVsn << ", vsn = " << vsn << ", lenrec = " << lenRec << std::endl;
				}
				ClearEventList();
				fullSpill=false; // WHY WAS THIS TRUE!?!? CRT
			}
			
			// Read the buffer.	After read, the vector eventList will 
			//contain pointers to all channels that fired in this buffer
			retval = ReadBuffer(&data[nWords_read], &bufLen);

			// If the return value is less than the error code, 
			//reading the buffer failed for some reason.	
			//Print error message and reset variables if necessary
			if(retval <= -100){
				if(is_verbose){ std::cout << "ReadSpill: READOUT PROBLEM " << retval << " in event " << counter << std::endl; }
				if(retval == -100){
					if(is_verbose){ std::cout << "ReadSpill:  Remove list " << lastVsn << " " << vsn << std::endl; }
					ClearEventList();
				}
				return false;
			}
			else if(retval > 0){		
				// Increment the total number of events observed 
				numEvents += retval;
			}
			
			// Update the variables that are keeping track of what has been
			// analyzed and increment the location in the current buffer
			lastVsn = vsn;
			nWords_read += lenRec;
		} 
		else if(vsn == 1000){ // Buffer with vsn 1000 was inserted with the time for superheavy exp't
			memcpy(&theTime, &data[nWords_read+2], sizeof(time_t));
			if(is_verbose){
				struct tm * timeinfo;
				timeinfo = localtime (&theTime);
				std::cout << "ReadSpill: Read wall clock time of " << asctime(timeinfo);
			}
			nWords_read += lenRec;
			continue;
		}
		else if(vsn == 9999){
			// End spill vsn
			break;
		}
		else{
			// Bail out if we have lost our place,		
			// (bad vsn) and process events	 
			std::cout << "ReadSpill: UNEXPECTED VSN " << vsn << std::endl;
			break;
		}
	} // while still have words

	if(nWords > TOTALREAD || nWords_read > TOTALREAD){
		std::cout << "ReadSpill: Values of nn - " << nWords << " nk - "<< nWords_read << " TOTALREAD - " << TOTALREAD << std::endl;
		return false;
	}

	// If the vsn is 9999 this is the end of a spill, signal this buffer
	// for processing and determine if the buffer is split between spills.
	if(vsn == 9999 || vsn == 1000){
		fullSpill = true;
		nWords_read += 2; // Skip it
		lastVsn = 0xFFFFFFFF;
	}

	// Check the number of read words
	if(is_verbose && nWords_read != nWords){
		std::cout << "ReadSpill: Received spill of " << nWords << " words, but read " << nWords_read << " words\n";
	}

	// If there are events to process, continue 
	if(numEvents > 0){
		if(fullSpill){ // if full spill process events
			// Sort the vector of pointers eventlist according to time
			//double lastTimestamp = (*(eventList.rbegin()))->time;

			// Sort the event list in time
			SortList();

			// Once the vector of pointers eventlist is sorted based on time,
			// begin the event processing in ScanList().
			// ScanList will also clear the event list for us.
			ScanList();

			// Once the eventlist has been scanned, reset the number 
			// of events to zero and update the event counter
			numEvents=0;
			evCount++;
			
			// Every once in a while (when evcount is a multiple of 1000)
			// print the time elapsed doing the analysis
			if((evCount % 1000 == 0 || evCount == 1) && theTime != 0){
				std::cout << std::endl << "ReadSpill: Data read up to poll status time " << ctime(&theTime);
			}
		}
		else {
			if(is_verbose){ std::cout << "ReadSpill: Spill split between buffers" << std::endl; }
			ClearEventList(); // This tosses out all events read into the deque so far
			return false; 
		}		
	}
	else if(retval != -10){
		if(is_verbose){ std::cout << "ReadSpill: bad buffer, numEvents = " << numEvents << std::endl; }
		ClearEventList(); // This tosses out all events read into the deque so far
		return false;
	}
	
	return true;		
}
