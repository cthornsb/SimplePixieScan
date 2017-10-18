/** \file Unpacker.cpp
 * \brief A class to handle the unpacking of UTK/ORNL style pixie16 data spills.
 *
 * This class is intended to be used as a replacement of pixiestd.cpp from Stan
 * Paulauskas's pixie_scan. The majority of function names and arguments are
 * preserved as much as possible while allowing for more standardized unpacking
 * of pixie16 data.
 * CRT
 *
 * \author C. R. Thornsberry
 * \date Feb. 12th, 2016
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string.h>
#include <time.h>
#include <cmath>
#include <algorithm>
#include <limits>

#include "Unpacker.hpp"
#include "XiaData.hpp"

void clearDeque(std::deque<XiaData*> &list){
	while(!list.empty()){
		delete list.front();
		list.pop_front();
	}
}

/** Scan the event list and sort it by timestamp.
  * \return Nothing.
  */
void Unpacker::TimeSort(){
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		sort(iter->begin(), iter->end(), &XiaData::compareTime);
	}
}

/** Scan the time sorted event list and package the events into a raw
  * event with a size governed by the event width.
  * \return True if the event list is not empty and false otherwise.
  */
bool Unpacker::BuildRawEventA(){
	if(!rawEvent.empty())
		ClearRawEvent();

	if(numRawEvt == 0){// This is the first rawEvent. Do some special processing.
		// Find the first XiaData event. The eventList is time sorted by module.
		// The first component of each deque will be the earliest time from that module.
		// The first event time will be the minimum of these first components.
		if(!GetFirstTime(firstTime))
			return false;
		std::cout << "BuildRawEvent: First start event time is " << firstTime << " clock ticks.\n";
		rawEventStartTime = firstTime;
	}
	else{ 
		// Move the event window forward to the next valid channel fire.
		if(!GetFirstTime(rawEventStartTime))
			return false;
	}

	if(rawEventMode == 1) // Negative time window.
		rawEventStartTime = rawEventStartTime - eventWidth;

	startEventTime = -1;
	rawEventStartTime = rawEventStartTime;
	rawEventStopTime = rawEventStartTime + eventWidth;

	unsigned int mod, chan;
	std::string type, subtype, tag;
	XiaData *current_event = NULL;

	if(useRawEventStats){
		chanTime.clear();
		chanID.clear();
		inEvent.clear();
	}

	// Loop over all time-sorted modules.
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		if(iter->empty())
			continue;
			
		// Loop over the list of channels that fired in this buffer
		while(!iter->empty()){
			current_event = iter->front();
			mod = current_event->modNum;
			chan = current_event->chanNum;
	
			if(mod > MAX_PIXIE_MOD || chan > MAX_PIXIE_CHAN){ // Skip this channel
				std::cout << "BuildRawEvent: Encountered non-physical Pixie ID (mod = " << mod << ", chan = " << chan << ")\n";
				delete current_event;
				iter->pop_front();
				continue;
			}

			double currtime = current_event->time;

			// Check for backwards time-skip. This is un-handled currently and needs fixed CRT!!!
			if(currtime < rawEventStartTime)
				std::cout << "BuildRawEvent: Detected backwards time-skip from start=" << rawEventStartTime << " to " << current_event->time << "???\n";

			// If the time difference between the current and previous event is 
			// larger than the event width, finalize the current event, otherwise
			// treat this as part of the current event
			if((currtime - rawEventStartTime) > eventWidth){ // 62 pixie ticks represents ~0.5 us
				break;
			}

			if(useRawEventStats){
				chanID.push_back(16*mod+chan);
				chanTime.push_back(current_event->time);
				inEvent.push_back(true);
			}

			// Update raw stats output with the new event before adding it to the raw event.
			RawStats(current_event);
	
			// Push this channel event into the rawEvent.
			rawEvent.push_back(current_event);
	
			// Remove this event from the event list but do not delete it yet.
			// Deleting of the channel events will be handled by clearing the rawEvent.
			iter->pop_front();
		}
	}

	numRawEvt++;
	
	return true;
}

/** Scan the time sorted event list and package the events into a raw
  * event with a size governed by the event width.
  * \return True if the start list is not empty and false otherwise.
  */
bool Unpacker::BuildRawEventB(){
	if(!rawEvent.empty())
		ClearRawEvent();

	unsigned int mod, chan;
	XiaData *current_event = NULL;

	if(startList.empty()){
		if(untriggeredMode){
			for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
				// Loop over the list of channels that fired in this module.
				while(!iter->empty()){
					current_event = iter->front();
					mod = current_event->modNum;
					chan = current_event->chanNum;
					
					if(useRawEventStats){
						chanID.push_back(16*mod+chan);
						chanTime.push_back(current_event->time);
						inEvent.push_back(false);
					}

					// Update raw stats output with the new event before adding it to the raw event.
					RawStats(current_event);
	
					// Push this channel event into the rawEvent.
					rawEvent.push_back(current_event);

					// Remove this event from the event list.
					iter->pop_front();
				}
			}

		}
		else if(!whitelist.empty()){ 
			// Loop over all time-sorted modules.
			int whitelistModCount = 0;
			for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
				if(!IsInWhitelist(whitelistModCount++, -1) || iter->empty())
					continue;
				
				// Loop over the list of channels that fired in this module.
				while(!iter->empty()){
					current_event = iter->front();
					mod = current_event->modNum;
					chan = current_event->chanNum;
					if(IsInWhitelist(current_event->modNum, current_event->chanNum)){
						if(useRawEventStats){
							chanID.push_back(16*mod+chan);
							chanTime.push_back(current_event->time);
							inEvent.push_back(false);
						}

						// Update raw stats output with the new event before adding it to the raw event.
						RawStats(current_event);
		
						// Push this channel event into the rawEvent.
						rawEvent.push_back(current_event);
					}
					else{ delete current_event; }
					// Remove this event from the event list.
					iter->pop_front();
				}
			}
		}
		else{ return false; }

		return !rawEvent.empty();
	}	

	if(numRawEvt == 0){// This is the first rawEvent. Do some special processing.
		// Find the first start event. 
		firstTime = startList.front()->time;
		std::cout << "BuildRawEvent: First start event time is " << firstTime << " clock ticks.\n";
	}

	XiaData *current_start = startList.front();
	startList.pop_front();

	// Put the current start event into the raw event.
	rawEvent.push_back(current_start);

	if(rawEventMode == 2) // Positive time window.
		rawEventStartTime = current_start->time + eventDelay;
	else if(rawEventMode == 3) // Negative time window.
		rawEventStartTime = current_start->time - (eventWidth + eventDelay);

	startEventTime = current_start->time;
	rawEventStartTime = rawEventStartTime;
	rawEventStopTime = rawEventStartTime + eventWidth;

	if(useRawEventStats){
		chanTime.clear();
		chanID.clear();
		inEvent.clear();
	}

	// Loop over all time-sorted modules.
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		if(iter->empty())
			continue;
			
		// Loop over the list of channels that fired in this module.
		while(!iter->empty()){
			current_event = iter->front();
			mod = current_event->modNum;
			chan = current_event->chanNum;
	
			if(mod > MAX_PIXIE_MOD || chan > MAX_PIXIE_CHAN){ // Skip this channel
				std::cout << "BuildRawEvent: Encountered non-physical Pixie ID (mod = " << mod << ", chan = " << chan << ")\n";
				delete current_event;
				iter->pop_front();
				continue;
			}

			// Get the trigger time of the current event.
			double difftime = current_event->time - rawEventStartTime;
			
			// Check for events in the event list which occur before the current start event.
			// Since the start list is time-ordered, if this event falls before the current
			// event window, then it will never fall into the following event windows.
			if(difftime <= 0 && !IsInWhitelist(mod, chan)){
				if(useRawEventStats){
					chanID.push_back(16*mod+chan);
					chanTime.push_back(current_event->time);
					inEvent.push_back(false);
				}
				delete current_event;
				iter->pop_front();
				continue;
			}
			
			// If the time difference between the current and previous event is 
			// larger than the event width, we are finished with the current module.
			if(difftime > eventWidth){ // 62 pixie ticks represents ~0.5 us
				break;
			}
			
			if(useRawEventStats){
				chanID.push_back(16*mod+chan);
				chanTime.push_back(current_event->time);
				inEvent.push_back(true);
			}

			// Update raw stats output with the new event before adding it to the raw event.
			RawStats(current_event);
	
			// Push this channel event into the rawEvent.
			rawEvent.push_back(current_event);
	
			// Remove this event from the event list but do not delete it yet.
			// Deleting of the channel events will be handled by clearing the rawEvent.
			iter->pop_front();
		}
	}
	
	numRawEvt++;
	
	return true;
}

/** Push an event into the event list.
  * \param[in]  event_ The XiaData to push onto the back of the event list.
  * \return True if the XiaData's module number is valid and false otherwise.
  */
bool Unpacker::AddEvent(XiaData *event_){
	if(event_->modNum > MAX_PIXIE_MOD){ return false; }
	
	// Check for the need to add a new deque to the event list.
	if(event_->modNum+1 > (unsigned short)eventList.size()){
		while ((unsigned short)eventList.size() < event_->modNum + 1) {
			eventList.push_back(std::deque<XiaData*>());
		}
	}

	if(rawEventMode >= 2 && (event_->modNum == startMod && event_->chanNum == startChan)) startList.push_back(event_);
	else eventList.at(event_->modNum).push_back(event_);
	
	return true;
}

/** Clear all events in the spill event list. WARNING! This method will delete all events in the
  * event list. This could cause seg faults if the events are used elsewhere.
  * \return Nothing.
  */	
void Unpacker::ClearEventList(){
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		clearDeque((*iter));
	}
}

/** Clear all events in the raw event list. WARNING! This method will delete all events in the
  * event list. This could cause seg faults if the events are used elsewhere.
  * \return Nothing.
  */
void Unpacker::ClearRawEvent(){
	clearDeque(rawEvent);
}

/** Get the minimum channel time from the event list.
  * \param[out] time The minimum time from the event list in system clock ticks.
  * \return True if the event list is not empty and false otherwise.
  */
bool Unpacker::GetFirstTime(double &time){
	if(IsEmpty())
		return false;

	time = std::numeric_limits<double>::max();
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		if(iter->empty())
			continue;
		if(iter->front()->time < time)
			time = iter->front()->time;
	}
	
	return true;
}

/** Check whether or not the eventList is empty.
  * \return True if the eventList is empty, and false otherwise.
  */
bool Unpacker::IsEmpty(){
	for(std::vector<std::deque<XiaData*> >::iterator iter = eventList.begin(); iter != eventList.end(); iter++){
		if(!iter->empty())
			return false;
	}
	return true;
}

/** Return a pointer to a new XiaData channel event.
  * \return A pointer to a new XiaData.
  */
XiaData *Unpacker::GetNewEvent(){ 
	return (new XiaData()); 
}

/** Process all events in the event list.
  * \param[in]  addr_ Pointer to a ScanInterface object. Unused by default.
  * \return Nothing.
  */
void Unpacker::ProcessRawEvent(ScanInterface *addr_/*=NULL*/){
	ClearRawEvent();
}

/** Called form ReadSpill. Scan the current spill and construct a list of
  * events which fired by obtaining the module, channel, trace, etc. of the
  * timestamped event. This method will construct the event list for
  * later processing.
  * \param[in]  buf    Pointer to an array of unsigned ints containing raw module data.
  * \param[out] bufLen The number of words in the module buffer.
  * \return The number of XiaData events read from the module buffer.
  */
int Unpacker::ReadSpillModule(unsigned int *buf){						
	unsigned int numEvents = 0;

	// Determine the number of words in the buffer
	unsigned int bufLen = buf[0];

	// Read the module number
	unsigned int modNum = buf[1];

	// Set the index of the first event in the module buffer.
	// The first two words are the buffer length and the module number respectively.
	unsigned int bufIndex = 2;

	//XiaData *lastVirtualChannel = NULL;

	if(bufLen > 0){ // Check if the buffer has data
		if(bufLen == 2){ // this is an empty module.
			return 0;
		}
		while( bufIndex < bufLen ){
			XiaData *currentEvt = new XiaData();

			//if(!currentEvt->readEventRevD(buf, bufIndex, modNum)){
			if(!currentEvt->readEventRevF(buf, bufIndex, modNum)){
				//std::cout << "ReadSpillModule: ERROR - XiaData::readBufferRevD failed to read event! Module=" << modNum << ", bufIndex=" << bufIndex << ", bufLen=" << bufLen << std::endl;
				std::cout << "ReadSpillModule: ERROR - XiaData::readBufferRevF failed to read event! Module=" << modNum << ", bufIndex=" << bufIndex << ", bufLen=" << bufLen << std::endl;
				delete currentEvt;
				continue;
			}

			// Does not handle multiple crates! CRT
			channel_counts[currentEvt->modNum][currentEvt->chanNum]++;
			
			// Add the event to the event list.
			AddEvent(currentEvt);
			numEvents++;
		}
	} 
	else{ // if buffer has data
		std::cout << "ReadSpillModule: ERROR - Spill has size zero!" << std::endl;
		return -100;
	}
	
	return numEvents;
}

/** Chekc if a specified module and channel pair is in the channel id whitelist.
  * \param[in]  mod The pixie module of the pair.
  * \param[in]  chan The pixie channel of the pair.
  * \return True if the module and channel pair is in the whitelist and false otherwise.
  *         If chan is negative, return true if the specified module has channels defined in the whitelist.
  */
bool Unpacker::IsInWhitelist(const int &mod, const int &chan){
	if(whitelist.empty()) return false;
	if(mod >= (int)whitelist.size()) return false;
	else if(chan < 0){
		if(whitelist.at(mod).empty()) return false;
		return true;
	}
	for(std::vector<int>::iterator iter = whitelist.at(mod).begin(); iter != whitelist.at(mod).end(); ++iter){
		if(chan == *iter) return true;
	}
	return false;
}

Unpacker::Unpacker() :
	eventWidth(62), // ~ 500 ns in 8 ns pixie clock ticks.
	eventDelay(0),
	debug_mode(false),
	running(true),
	interface(NULL),
	TOTALREAD(1000000), // Maximum number of data words to read.
	maxWords(131072), // Maximum number of data words for revision D.	
	numRawEvt(0), // Count of raw events read from file.
	firstTime(0),
	rawEventMode(0), // The raw event building method to use.
	startMod(0),
	startChan(0),
	startEventTime(0),
	rawEventStartTime(0),
	rawEventStopTime(0),
	useRawEventStats(false),
	untriggeredMode(false)
{
	for(unsigned int i = 0; i <= MAX_PIXIE_MOD; i++){
		for(unsigned int j = 0; j <= MAX_PIXIE_CHAN; j++){
			channel_counts[i][j] = 0;
		}
	}
}

/// Destructor.
Unpacker::~Unpacker(){
	ClearRawEvent();
	ClearEventList();
}

/// Return a pointer to the vector of channel times from the current raw event.	
std::vector<double> *Unpacker::GetRawEventChanTime(){ 
	useRawEventStats = true;
	return &chanTime; 
}

/// Return a pointer to the vector of channel IDs from the current raw event.
std::vector<int> *Unpacker::GetRawEventChanID(){ 
	useRawEventStats = true;
	return &chanID; 
}

/// Return a pointer to the vector of channel flags from the current raw event.
std::vector<bool> *Unpacker::GetRawEventFlag(){ 
	useRawEventStats = true;
	return &inEvent; 
}

/// Return a pointer to the time of the current start event.
double *Unpacker::GetStartEventTime(){ 
	useRawEventStats = true;
	return &startEventTime; 
}

/// Return a pointer to the start time of the current raw event window.
double *Unpacker::GetRawEventStartTime(){ 
	useRawEventStats = true;
	return &rawEventStartTime; 
}

/// Return a pointer to the stop time of the current raw event window.
double *Unpacker::GetRawEventStopTime(){ 
	useRawEventStats = true;
	return &rawEventStopTime; 
}

/** ReadSpill is responsible for constructing a list of pixie16 events from
  * a raw data spill. This method performs sanity checks on the spill and
  * calls ReadBuffer in order to construct the event list.
  * \param[in]  data       Pointer to an array of unsigned ints containing the spill data.
  * \param[in]  nWords     The number of words in the array.
  * \param[in]  is_verbose Toggle the verbosity flag on/off.
  * \return True if the spill was read successfully and false otherwise.
  */	
bool Unpacker::ReadSpill(unsigned int *data, unsigned int nWords, bool is_verbose/*=true*/){
	const unsigned int maxVsn = 14; // No more than 14 pixie modules per crate
	unsigned int nWords_read = 0;
	
	//static clock_t clockBegin; // Initialization time
	//time_t tmsBegin;

	int retval = 0; // return value from various functions
	
	// Various event counters 
	unsigned long numEvents = 0;
	static int counter = 0; // the number of times this function is called
	static int evCount;	 // the number of times data is passed to ScanList
	static unsigned int lastVsn; // the last vsn read from the data
	time_t theTime = 0;

	// Initialize the scan program before the first event 
	if(counter==0){ lastVsn=-1; } // Set last vsn to -1 so we expect vsn 0 first 	
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
			retval = ReadSpillModule(&data[nWords_read]);

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
				/*struct tm * timeinfo;
				timeinfo = localtime (&theTime);
				std::cout << "ReadSpill: Read wall clock time of " << asctime(timeinfo);*/
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
			TimeSort();

			// Once the vector of pointers eventlist is sorted based on time,
			// begin the event processing in ScanList().
			// ScanList will also clear the event list for us.
			if(rawEventMode <= 1){
				while(BuildRawEventA()){ // Build a new raw event and process it.
					ProcessRawEvent(interface);
				}
			}
			else{
				while(BuildRawEventB()){ // Build a new raw event and process it.
					ProcessRawEvent(interface);
				}
			}

			ClearEventList();
			
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

/** ReadRawEvent assumes that the events in the incoming data are already grouped
  * into a raw event. This method performs sanity checks on the raw event and calls
  * ReadBuffer in order to construct the event list.
  * \param[in]  data       Pointer to an array of unsigned ints containing the spill data.
  * \param[in]  nWords     The number of words in the array.
  * \param[in]  is_verbose Toggle the verbosity flag on/off.
  * \return True if the spill was read successfully and false otherwise.
  */	
bool Unpacker::ReadRawEvent(unsigned int *data, unsigned int nWords, bool is_verbose/*=true*/){
	if(!rawEvent.empty())
		ClearRawEvent();

	// Reset the minimum times.
	if(numRawEvt == 0)
		firstTime = std::numeric_limits<double>::max();
	rawEventStartTime = std::numeric_limits<double>::max();
	rawEventStopTime = std::numeric_limits<double>::min();

	// Set the index of the first event in the module buffer.
	unsigned int bufIndex = 0;

	while( bufIndex < nWords ){
		XiaData *currentEvt = GetNewEvent();
	
		if(!currentEvt->readEvent(data, bufIndex)){
			std::cout << "ReadRawEvent: ERROR - readEvent failed to read event! numRawEvt=" << numRawEvt << ", bufIndex=" << bufIndex << ", nWords=" << nWords << std::endl;
			delete currentEvt;
			continue;
		}
		
		// Check for the minimum time in this raw event.
		if(currentEvt->time < rawEventStartTime) 
			rawEventStartTime = currentEvt->time;
			
		// Check for the maximum time in this raw event.
		if(currentEvt->time > rawEventStopTime) 
			rawEventStopTime = currentEvt->time;

		// Does not handle multiple crates! CRT
		channel_counts[currentEvt->modNum][currentEvt->chanNum]++;

		// Update raw stats output with the new event before adding it to the raw event.
		RawStats(currentEvt);			
		
		// Push this channel event into the rawEvent.
		rawEvent.push_back(currentEvt);
		
		// Check for the end of the raw event.
		if(bufIndex < nWords && data[bufIndex] == 0xFFFFFFFF){
			// Print the time of the very first raw event.
			if(numRawEvt == 0){
				firstTime = rawEventStartTime;
				std::cout << "ReadRawEvent: First event time is " << firstTime << " clock ticks.\n";
			}

			// Process the event.
			ProcessRawEvent(interface);

			// Increment the number of raw events which have been read.
			numRawEvt++;		
			
			// Skip the delimiter.
			bufIndex++; 
		}
	}
	
	return true;
}

/** Write all recorded channel counts to a file.
  * \return Nothing.
  */
void Unpacker::Write(){
	std::ofstream count_output("counts.dat");
	if(count_output.good()){
		for(unsigned int i = 0; i <= MAX_PIXIE_MOD; i++){
			for(unsigned int j = 0; j <= MAX_PIXIE_CHAN; j++){
				count_output << i << "\t" << j << "\t" << channel_counts[i][j] << std::endl;
			}
		}
		count_output.close();
	}
}

/** Clear event list and raw event.
  * \return Nothing.
  */
void Unpacker::Clear(){
	ClearRawEvent();
	ClearEventList();
}

/** Add a pixie module & channel pair to the event whitelist.
  * \param[in]  mod The pixie module of the pair.
  * \param[in]  chan The pixie channel of the pair.
  * \return Nothing.
  */
void Unpacker::AddToWhitelist(const int &mod, const int &chan){
	// Check for the need to add a new vector to the whitelist.
	while ((int)whitelist.size() < mod+1) {
		whitelist.push_back(std::vector<int>());
	}
	whitelist.at(mod).push_back(chan);
}
