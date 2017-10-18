#include <cmath>
#include <string.h>

#include "XiaData.hpp"

/////////////////////////////////////////////////////////////////////
// XiaData
/////////////////////////////////////////////////////////////////////

/// Default constructor.
XiaData::XiaData(){
	adcTrace = NULL;
	qdcValue = NULL;
	clear();
}

/// Constructor from a pointer to another XiaData.
XiaData::XiaData(XiaData *other_){
	adcTrace = NULL;
	qdcValue = NULL;
	clear();

	energy = other_->energy; 
	time = other_->time;

	modNum = other_->modNum;
	chanNum = other_->chanNum;
	cfdTime = other_->cfdTime;
	eventTimeLo = other_->eventTimeLo;
	eventTimeHi = other_->eventTimeHi;

	virtualChannel = other_->virtualChannel;
	pileupBit = other_->pileupBit;
	saturatedBit = other_->saturatedBit;
	cfdForceTrig = other_->cfdForceTrig; 
	cfdTrigSource = other_->cfdTrigSource; 
	outOfRange = other_->outOfRange;
	saturatedTrace = other_->saturatedTrace;

	// Copy the ADC trace, if enabled.
	if(other_->traceLength > 0)
		copyTrace((char *)other_->adcTrace, other_->traceLength);

	// Copy the onboard QDCs, if enabled.
	if(other_->numQdcs > 0)
		copyQDCs((char *)other_->qdcValue, other_->numQdcs);
}

XiaData::~XiaData(){
	clearTrace();
	clearQDCs();
}

/// Fill the trace by reading from a character array.
void XiaData::copyTrace(char *ptr_, const unsigned short &size_){
	if(size_ == 0) return;
	else if(adcTrace == NULL || size_ != traceLength){
		clearTrace();
		traceLength = size_;
		adcTrace = new unsigned short[traceLength];
	}
	memcpy((char *)adcTrace, ptr_, traceLength*2);
}

/// Fill the QDC array by reading a character array.
void XiaData::copyQDCs(char *ptr_, const unsigned short &size_){
	if(size_ == 0) return;
	else if(qdcValue == NULL || size_ != numQdcs){
		clearQDCs();
		numQdcs = size_;
		qdcValue = new unsigned int[numQdcs];
	}
	memcpy((char *)qdcValue, ptr_, numQdcs*4);
}

void XiaData::clear(){
	energy = 0.0; 
	time = 0.0;
	
	modNum = 0;
	chanNum = 0;
	cfdTime = 0;
	eventTimeLo = 0;
	eventTimeHi = 0;

	virtualChannel = false;
	pileupBit = false;
	saturatedBit = false;
	cfdForceTrig = false; 
	cfdTrigSource = false; 
	outOfRange = false;
	saturatedTrace = false;	

	clearTrace();
	clearQDCs();
}

/// Delete the trace.
void XiaData::clearTrace(){
	if(adcTrace != NULL)
		delete[] adcTrace;
	
	traceLength = 0;
	adcTrace = NULL;
}

/// Delete the QDC array.
void XiaData::clearQDCs(){
	if(qdcValue != NULL)
		delete[] qdcValue;
	
	numQdcs = 0;
	qdcValue = NULL;
}

/// Print event information to the screen.
void XiaData::print(){
	std::cout << " energy:	  " << this->energy << std::endl;
	std::cout << " time:	    " << this->time << std::endl;
	std::cout << " traceLength: " << this->traceLength << std::endl;
	std::cout << " numQdcs:	 " << this->numQdcs << std::endl;
	std::cout << " slotNum:	 " << this->slotNum << std::endl;
	std::cout << " modNum:	  " << this->modNum << std::endl;
	std::cout << " chanNum:	 " << this->chanNum << std::endl;
	std::cout << " cfdTime:	 " << this->cfdTime << std::endl;
	std::cout << " eventTimeLo: " << this->eventTimeLo << std::endl;
	std::cout << " eventTimeHi: " << this->eventTimeHi << std::endl;
	print2();
}

/** Responsible for decoding individual pixie events from a binary input file.
  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
  * \param[in]  module	  The current module number being scanned.
  * \param[out] bufferIndex The current index in the module buffer.
  * \return Only false currently. This method is only a stub.
  */
bool XiaData::readEventRevD(unsigned int *buf, unsigned int &bufferIndex, unsigned int module/*=9999*/){
	return false;
}

/** Responsible for decoding individual pixie events from a binary input file.
  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
  * \param[in]  module	  The current module number being scanned.
  * \param[out] bufferIndex The current index in the module buffer.
  * \return True if the event was successfully read, or false otherwise.
  */
bool XiaData::readEventRevF(unsigned int *buf, unsigned int &bufferIndex, unsigned int module/*=9999*/){	
	// Decoding event data... see pixie16app.c
	// buf points to the start of channel data
	chanNum	    =  (buf[bufferIndex] & 0x0000000F);
	slotNum	    =  (buf[bufferIndex] & 0x000000F0) >> 4;
	crateNum	   =  (buf[bufferIndex] & 0x00000F00) >> 8;
	headerLength   =  (buf[bufferIndex] & 0x0001F000) >> 12;
	eventLength	=  (buf[bufferIndex] & 0x1FFE0000) >> 17;
	virtualChannel = ((buf[bufferIndex] & 0x20000000) != 0);
	saturatedBit   = ((buf[bufferIndex] & 0x40000000) != 0);
	pileupBit	  = ((buf[bufferIndex] & 0x80000000) != 0);	

	eventTimeLo =  buf[bufferIndex + 1];
	eventTimeHi =  buf[bufferIndex + 2] & 0x0000FFFF;
	cfdTime	 = (buf[bufferIndex + 2] & 0xFFFF0000) >> 16;
	energy	  =  buf[bufferIndex + 3] & 0x0000FFFF;
	traceLength = (buf[bufferIndex + 3] & 0x7FFF0000) >> 16;
	outOfRange = ((buf[bufferIndex] & 0x80000000) != 0);

	// Handle saturated filter energy.
	if(saturatedBit){ energy = 32767; }
	
	const unsigned long long HIGH_BIT_MULT = 0x00000000FFFFFFFF;
	
	// Calculate the 48-bit trigger time.
	time = eventTimeLo + eventTimeHi * HIGH_BIT_MULT;

	if(module == 9999) modNum = slotNum;
	else modNum = module;

	// Handle multiple crates
	modNum += 100 * crateNum;

	// Rev. D header lengths not clearly defined in pixie16app_defs
	//! magic numbers here for now
	if(headerLength == 1){
		// this is a manual statistics block inserted by the poll program
		/*stats.DoStatisticsBlock(&buf[bufferIndex + 1], modNum);
		numEvents = -10;*/
		
		// Advance to next event.
		bufferIndex += eventLength;
		return false;
	}
	
	// Check that the header length is valid.
	if(headerLength != 4 && headerLength != 8 && headerLength != 12 && headerLength != 16){
		std::cout << "ReadEventRevF: Unexpected header length: " << headerLength << std::endl;
		std::cout << "ReadEventRevF:  Module " << modNum << std::endl;
		std::cout << "ReadEventRevF:  CHAN:SLOT:CRATE " << chanNum << ":" << slotNum << ":" << crateNum << std::endl;
		
		// Check for zero event length.
		if(eventLength == 0){
			std::cout << "ReadEventRevF: ERROR! Encountered zero event length. Moving ahead one word.\n";
			bufferIndex += 1;
			return false;
		}
		
		// Advance to next event.
		bufferIndex += eventLength;
		return false;
	}

	// One last check on the event length.
	if( traceLength / 2 + headerLength != eventLength ){
		std::cout << "ReadEventRevF: Bad event length (" << eventLength << ") does not correspond with length of header (";
		std::cout << headerLength << ") and length of trace (" << traceLength << ")" << std::endl;
		
		// Advance to next event.
		bufferIndex += eventLength;
		return false;
	}

	// Move the buffer index past the header.
	bufferIndex += 4;

	if(headerLength == 8 || headerLength == 16){
		// Skip the onboard partial sums for now 
		// trailing, leading, gap, baseline
		bufferIndex += 4;
	}

	if(headerLength >= 12){ // Copy the QDCs.
		copyQDCs((char *)&buf[bufferIndex], 8);
		bufferIndex += 8;
	}

	/*if(currentEvt->virtualChannel){
		DetectorLibrary* modChan = DetectorLibrary::get();

		currentEvt->modNum += modChan->GetPhysicalModules();
		if(modChan->at(modNum, chanNum).HasTag("construct_trace")){
			lastVirtualChannel = currentEvt;
		}
	}*/

	// Check if trace data follows the channel header
	if( traceLength > 0 ){
		/*if(currentEvt->saturatedBit)
			currentEvt->trace.SetValue("saturation", 1);*/

		/*if( lastVirtualChannel != NULL && lastVirtualChannel->traceLength == 0 ){		
			lastVirtualChannel->assign(0);
		}*/
		// Read the trace data (2-bytes per sample, i.e. 2 samples per word)
		copyTrace((char *)&buf[bufferIndex], traceLength);
		bufferIndex += (traceLength / 2);
	}
	
	return true;
}

/// Get the size of the XiaData event when written to disk by ::writeEventRevF (in 4-byte words).
size_t XiaData::getEventLengthRevF(){
	size_t eventLength = 4;
	if(numQdcs > 0) eventLength += numQdcs; // Account for the onboard QDCs.
	if(traceLength > 0) eventLength += traceLength/2; // Account for the ADC trace.
	return eventLength;
}

/** Write a pixie style event to a binary output file. Output data may
  * be written to both an ofstream and a character array. One of the
  * pointers must not be NULL.
  * 
  * \param[in] file_ Pointer to an ofstream output binary file.
  * \param[in] array_ Pointer to a character array into which data will be written.
  * \return The number of bytes written to the file upon success and -1 otherwise.
  */
int XiaData::writeEventRevF(std::ofstream *file_, char *array_){
	if((!file_ && !array_) || (file_ && !file_->good())) return -1;

	unsigned int crateNum = 0x0; // Fixed value for now.
	unsigned int chanIdentifier = 0xFFFFFFFF;
	unsigned int eventTimeHiWord = 0xFFFFFFFF;
	unsigned int eventEnergyWord = 0xFFFFFFFF;

	unsigned int eventLength = (unsigned int)getEventLengthRevF();
	unsigned int headLength = eventLength - (unsigned int)traceLength/2;
	
	// Build up the channel identifier.
	chanIdentifier &= ~(0x0000000F & (chanNum));	          // Pixie channel number
	chanIdentifier &= ~(0x000000F0 & (modNum << 4));	      // Pixie module number (NOT the slot number)
	chanIdentifier &= ~(0x00000F00 & (crateNum << 8));	    // Crate number
	chanIdentifier &= ~(0x0001F000 & (headLength << 12));	 // Header length
	chanIdentifier &= ~(0x1FFE0000 & (eventLength << 17));	// Event length
	chanIdentifier &= ~(0x20000000 & (virtualChannel << 29)); // Virtual channel bit
	chanIdentifier &= ~(0x40000000 & (saturatedBit << 30));   // Saturated channel bit
	chanIdentifier &= ~(0x80000000 & (pileupBit << 31));	  // Pileup bit
	chanIdentifier = ~chanIdentifier;
	
	// Build up the high event time and CFD time.
	eventTimeHiWord &= ~(0x0000FFFF & (eventTimeHi));
	eventTimeHiWord &= ~(0xFFFF0000 & (cfdTime << 16));
	eventTimeHiWord = ~eventTimeHiWord;
	
	// Build up the event energy.
	eventEnergyWord &= ~(0x0000FFFF & (energy));
	eventEnergyWord &= ~(0xFFFF0000 & (traceLength << 16));
	eventEnergyWord = ~eventEnergyWord;
	
	if(file_){
		// Write data to the output file.
		file_->write((char *)&chanIdentifier, 4);
		file_->write((char *)&eventTimeLo, 4);
		file_->write((char *)&eventTimeHiWord, 4);
		file_->write((char *)&eventEnergyWord, 4);
	}
	
	if(array_){
		// Write data to the character array.
		memcpy(array_, (char *)&chanIdentifier, 4);
		memcpy(&array_[4], (char *)&eventTimeLo, 4);
		memcpy(&array_[8], (char *)&eventTimeHiWord, 4);
		memcpy(&array_[12], (char *)&eventEnergyWord, 4);
	}

	int numBytes = 16;

	// Write the onbard QDCs, if enabled.
	if(numQdcs > 0){
		if(file_) file_->write((char *)qdcValue, numQdcs*4);
		if(array_) memcpy(&array_[numBytes], (char *)qdcValue, numQdcs*4);
		numBytes += numQdcs*4;
	}

	// Write the ADC trace, if enabled.
	if(traceLength != 0){ // Write the trace.
		if(file_) file_->write((char *)adcTrace, traceLength*2);
		if(array_) memcpy(&array_[numBytes], (char *)adcTrace, traceLength*2);
		numBytes += traceLength*2;
	}
	
	return numBytes;
}

/////////////////////////////////////////////////////////////////////
// ChannelEvent
/////////////////////////////////////////////////////////////////////

/// Default constructor.
ChannelEvent::ChannelEvent(){
	Clear();
}

/// Constructor from a XiaData. ChannelEvent will take ownership of the XiaData.
ChannelEvent::ChannelEvent(XiaData *event_) : XiaData(event_) {
	Clear();
}

ChannelEvent::~ChannelEvent(){
	if(cfdvals) delete[] cfdvals;
}

float ChannelEvent::ComputeBaseline(){
	if(traceLength == 0){ return -9999; }
	if(baseline > 0){ return baseline; }

	// Find the baseline.
	double tempbaseline = 0.0;
	double tempstddev = 0.0;
	size_t sample_size = (15 <= traceLength ? 15:traceLength);
	for(size_t i = 0; i < sample_size; i++){
		tempbaseline += adcTrace[i];
		tempstddev += std::pow(adcTrace[i], 2.0);
	}
	tempbaseline = tempbaseline/sample_size;
	
	// Calculate the standard deviation of the baseline.
	tempstddev = std::sqrt((tempstddev/sample_size) - tempbaseline*tempbaseline);

	baseline = float(tempbaseline);
	stddev = float(tempstddev);	

	// Find the maximum ADC value and the maximum bin.
	max_ADC = 0;
	for(size_t i = 0; i < traceLength; i++){
		if(adcTrace[i]-baseline > max_ADC){ 
			max_ADC = adcTrace[i]-baseline;
			max_index = i;
		}
		if(adcTrace[i] >= 4095) saturatedTrace = true;
	}

	// Find the pulse maximum by fitting with a third order polynomial.
	if(adcTrace[max_index-1] >= adcTrace[max_index+1]) // Favor the left side of the pulse.
		maximum = calculateP3(max_index-2, &adcTrace[max_index-2], cfdPar) - baseline;
	else // Favor the right side of the pulse.
		maximum = calculateP3(max_index-1, &adcTrace[max_index-1], cfdPar) - baseline;

	return baseline;
}

float ChannelEvent::IntegratePulse(const size_t &start_/*=0*/, const size_t &stop_/*=0*/, bool calcQdc2/*=false*/){
	if(traceLength == 0 || baseline < 0.0){ return -9999; }
	
	size_t stop = (stop_ == 0?traceLength:stop_);

	// Check for out of bounds of trace.
	if(stop >= traceLength) return -9999;
	
	// Check for start index greater than stop index.
	if(start_+1 >= stop) return -9999;

	if(calcQdc2){
		qdc2 = 0.0;
		for(size_t i = start_+1; i < stop; i++){ // Integrate using trapezoidal rule.
			qdc2 += 0.5*(adcTrace[i-1] + adcTrace[i]) - baseline;
		}
		return qdc2;
	}

	qdc = 0.0;
	for(size_t i = start_+1; i < stop; i++){ // Integrate using trapezoidal rule.
		qdc += 0.5*(adcTrace[i-1] + adcTrace[i]) - baseline;
	}

	return qdc;
}

/// Perform traditional CFD analysis on the waveform.
float ChannelEvent::AnalyzeCFD(const float &F_/*=0.5*/, const size_t &D_/*=1*/, const size_t &L_/*=1*/){
	if(traceLength == 0 || baseline < 0){ return -9999; }
	if(!cfdvals)
		cfdvals = new float[traceLength];
	
	float cfdMinimum = 9999;
	size_t cfdMinIndex = 0;
	
	phase = -9999;

	// Compute the cfd waveform.
	for(size_t cfdIndex = 0; cfdIndex < traceLength; ++cfdIndex){
		cfdvals[cfdIndex] = 0.0;
		if(cfdIndex >= L_ + D_ - 1){
			for(size_t i = 0; i < L_; i++)
				cfdvals[cfdIndex] += F_ * (adcTrace[cfdIndex - i]-baseline) - (adcTrace[cfdIndex - i - D_]-baseline);
		}
		if(cfdvals[cfdIndex] < cfdMinimum){
			cfdMinimum = cfdvals[cfdIndex];
			cfdMinIndex = cfdIndex;
		}
	}

	// Find the zero-crossing.
	if(cfdMinIndex > 0){
		// Find the zero-crossing.
		for(size_t cfdIndex = cfdMinIndex-1; cfdIndex >= 0; cfdIndex--){
			if(cfdvals[cfdIndex] >= 0.0 && cfdvals[cfdIndex+1] < 0.0){
				phase = cfdIndex - cfdvals[cfdIndex]/(cfdvals[cfdIndex+1]-cfdvals[cfdIndex]);
				break;
			}
		}
	}

	return phase;
}

/// Perform polynomial CFD analysis on the waveform.
float ChannelEvent::AnalyzePolyCFD(const float &F_/*=0.5*/){
	if(traceLength == 0 || baseline < 0){ return -9999; }

	float threshold = F_*maximum + baseline;

	phase = -9999;
	for(cfdIndex = max_index; cfdIndex > 0; cfdIndex--){
		if(adcTrace[cfdIndex-1] < threshold && adcTrace[cfdIndex] >= threshold){
			// Fit the rise of the trace to a 2nd order polynomial.
			calculateP2(cfdIndex-1, &adcTrace[cfdIndex-1], &cfdPar[4]);
			
			// Calculate the phase of the trace.
			if(cfdPar[6] != 0)
				phase = (-cfdPar[5]+std::sqrt(cfdPar[5]*cfdPar[5] - 4*cfdPar[6]*(cfdPar[4] - threshold)))/(2*cfdPar[6]);
			else
				phase = (threshold-cfdPar[4])/cfdPar[5];

			break;
		}
	}

	return phase;
}

void ChannelEvent::Clear(){
	hiresTime = 0.0;

	phase = -9999;
	maximum = -9999;
	baseline = -9999;
	stddev = -9999;
	qdc = -9999;
	qdc2 = -9999;
	max_index = 0;
	max_ADC = 0;

	valid_chan = false;
	ignore = false;

	cfdvals = NULL;
}

/** Responsible for decoding ChannelEvents from a binary input file.
  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
  * \param[in]  modNum	 The current module number being scanned.
  * \param[out] bufferIndex The current index in the module buffer.
  * \return True if the event was successfully read, or false otherwise.
  */
bool ChannelEvent::readEvent(unsigned int *buf, unsigned int &bufferIndex){
	unsigned short headLength, chanFlags;

	chanNum  =  (buf[bufferIndex] & 0x0000000F);
	modNum   =  (buf[bufferIndex] & 0x000000F0) >> 4;
	crateNum =  (buf[bufferIndex] & 0x00000F00) >> 8;
	headLength = (buf[bufferIndex] & 0x0000F000) >> 12;
	traceLength = (buf[bufferIndex] & 0xFFFF0000) >> 16;
	
	chanFlags = (buf[bufferIndex + 1] & 0x0000FFFF);
	cfdTime = (buf[bufferIndex + 1] & 0xFFFF0000) >> 16;

	// Decode the channel flags.
	if(chanFlags & 1) virtualChannel = true;
	if(chanFlags & 2) pileupBit = true;
	if(chanFlags & 4) saturatedBit = true;
	if(chanFlags & 8) cfdForceTrig = true;
	if(chanFlags & 16) cfdTrigSource = true;
	if(chanFlags & 32) outOfRange = true;
	if(chanFlags & 64) valid_chan = true;
	/*if(chanFlags & 128) valid_chan = true;
	if(chanFlags & 256) valid_chan = true;
	if(chanFlags & 512) valid_chan = true;
	if(chanFlags & 1024) valid_chan = true;
	if(chanFlags & 2048) valid_chan = true;
	if(chanFlags & 4096) valid_chan = true;
	if(chanFlags & 8192) valid_chan = true;
	if(chanFlags & 16384) valid_chan = true;
	if(chanFlags & 32768) valid_chan = true;*/

	// The slot number shouldn't be needed, but just for completeness.
	slotNum = modNum;

	eventTimeLo =  buf[bufferIndex + 2];
	eventTimeHi =  buf[bufferIndex + 3] & 0x0000FFFF;
	energy	  = (buf[bufferIndex + 3] & 0xFFFF0000) >> 16;

	// Calculate the 48-bit trigger time.	
	time = eventTimeLo + eventTimeHi * 0xFFFFFFFF;

	memcpy((char *)&hiresTime, (char *)&buf[bufferIndex + 4], 8);
	max_ADC   = (buf[bufferIndex + 6] & 0x0000FFFF);
	max_index = (buf[bufferIndex + 6] & 0xFFFF0000) >> 16;
	memcpy((char *)&phase, (char *)&buf[bufferIndex + 7], 4);
	memcpy((char *)&baseline, (char *)&buf[bufferIndex + 8], 4);
	memcpy((char *)&stddev, (char *)&buf[bufferIndex + 9], 4);
	memcpy((char *)&maximum, (char *)&buf[bufferIndex + 10], 4);
	memcpy((char *)&qdc, (char *)&buf[bufferIndex + 11], 4);
	if(headLength == 13)
		memcpy((char *)&qdc2, (char *)&buf[bufferIndex + 12], 4);

	bufferIndex += headLength;

	// Read the ADC trace, if enabled.
	if(traceLength != 0){ // Read the trace.
		copyTrace((char *)&buf[bufferIndex], traceLength);
		bufferIndex += (traceLength / 2);
	}

	return true;
}

/** Write a ChannelEvent to a binary output file. Output data may
  * be written to both an ofstream and a character array. One of the
  * pointers must not be NULL.
  * 
  * \param[in] file_ Pointer to an ofstream output binary file.
  * \param[in] array_ Pointer to a character array into which data will be written.
  * \param[in] recordTrace_ If set to true, the ADC trace will be written to output.
  * \return The number of bytes written to the file upon success and -1 otherwise.
  */
int ChannelEvent::writeEvent(std::ofstream *file_, char *array_, bool recordTrace_/*=false*/){
	if((!file_ && !array_) || (file_ && !file_->good())) return -1;

	const unsigned short tlength = (!recordTrace_) ? 0x0 : traceLength;
	
	unsigned short chanIdentifier = 0xFFFF;
	unsigned int eventTimeHiWord = 0xFFFFFFFF;
	
	unsigned short headLength = 12;

	// Add a word to the header length if qdc2 is used.
	if(qdc2 > 0) headLength += 1;

	// Build up the channel identifier.
	chanIdentifier &= ~(0x000F & (chanNum));	   // Pixie channel number
	chanIdentifier &= ~(0x00F0 & (modNum << 4));   // Pixie module number (NOT the slot number)
	chanIdentifier &= ~(0x0F00 & (crateNum << 8)); // Crate number
	chanIdentifier &= ~(0xF000 & (headLength << 12));
	chanIdentifier = ~chanIdentifier;

	// Build up the channel flags half-word.
	unsigned short chanFlags = 0x0;
	if(virtualChannel) chanFlags |= 1;
	if(pileupBit)	  chanFlags |= 2;
	if(saturatedBit)   chanFlags |= 4;
	if(cfdForceTrig)   chanFlags |= 8;
	if(cfdTrigSource)  chanFlags |= 16;
	if(outOfRange)	 chanFlags |= 32;
	if(valid_chan)	 chanFlags |= 64;
	/*if(statement) chanFlags |= 128;
	if(statement) chanFlags |= 256;
	if(statement) chanFlags |= 512;
	if(statement) chanFlags |= 1024;
	if(statement) chanFlags |= 2048;
	if(statement) chanFlags |= 4096;
	if(statement) chanFlags |= 8192;
	if(statement) chanFlags |= 16384;
	if(statement) chanFlags |= 32768;*/
	
	// Build up the high event time and CFD time.
	eventTimeHiWord &= ~(0x0000FFFF & (eventTimeHi));
	eventTimeHiWord &= ~(0xFFFF0000 & (energy << 16));
	eventTimeHiWord = ~eventTimeHiWord;

	if(file_){
		// Write data to the output file.
		file_->write((char *)&chanIdentifier, 2);
		file_->write((char *)&tlength, 2);
		file_->write((char *)&chanFlags, 2);
		file_->write((char *)&cfdTime, 2);
		file_->write((char *)&eventTimeLo, 4);
		file_->write((char *)&eventTimeHiWord, 4);
		file_->write((char *)&hiresTime, 8);
		file_->write((char *)&max_index, 2);
		file_->write((char *)&max_ADC, 2);
		file_->write((char *)&phase, 4);
		file_->write((char *)&baseline, 4);
		file_->write((char *)&stddev, 4);
		file_->write((char *)&maximum, 4);
		file_->write((char *)&qdc, 4);
		if(qdc2 > 0)
			file_->write((char *)&qdc2, 4);
	}
	
	if(array_){
		// Write data to the character array.
		memcpy(array_, (char *)&chanIdentifier, 2);
		memcpy(&array_[2], (char *)&tlength, 2);
		memcpy(&array_[4], (char *)&chanFlags, 2);
		memcpy(&array_[6], (char *)&cfdTime, 2);
		memcpy(&array_[8], (char *)&eventTimeLo, 4);
		memcpy(&array_[12], (char *)&eventTimeHiWord, 4);
		memcpy(&array_[16], (char *)&hiresTime, 8);
		memcpy(&array_[24], (char *)&max_index, 2);
		memcpy(&array_[26], (char *)&max_ADC, 2);
		memcpy(&array_[28], (char *)&phase, 4);
		memcpy(&array_[32], (char *)&baseline, 4);
		memcpy(&array_[36], (char *)&stddev, 4);
		memcpy(&array_[40], (char *)&maximum, 4);
		memcpy(&array_[44], (char *)&qdc, 4);
		if(qdc2 > 0)
			memcpy(&array_[48], (char *)&qdc2, 4);
	}	

	int numBytes = headLength * 4;

	// Write the ADC trace, if enabled.
	if(recordTrace_ && traceLength != 0){
		if(file_) file_->write((char *)adcTrace, traceLength*2);
		if(array_) memcpy(&array_[numBytes], (char *)adcTrace, traceLength*2);
		numBytes += traceLength*2;
	}

	return numBytes;
}

/// Print additional information to the screen.
void ChannelEvent::print2(){
	std::cout << " hiresTime:   " << this->hiresTime << std::endl;
	std::cout << " phase:	   " << this->phase << std::endl;
	std::cout << " baseline:	" << this->baseline << std::endl;
	std::cout << " stddev:	  " << this->stddev << std::endl;
	std::cout << " maximum:	 " << this->maximum << std::endl;
	std::cout << " qdc:	     " << this->qdc << std::endl;
}

double calculateP2(const short &x0, unsigned short *y, double *p){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		x1[i] = (x0+i);
		x2[i] = std::pow(x0+i, 2);
	}

	double denom = (x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(x2[2]-x2[1]*1) + x2[0]*(x1[2]-x1[1]*1);

	p[0] = (y[0]*(x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(y[1]*x2[2]-x2[1]*y[2]) + x2[0]*(y[1]*x1[2]-x1[1]*y[2]))/denom;
	p[1] = ((y[1]*x2[2]-x2[1]*y[2]) - y[0]*(x2[2]-x2[1]*1) + x2[0]*(y[2]-y[1]*1))/denom;
	p[2] = ((x1[1]*y[2]-y[1]*x1[2]) - x1[0]*(y[2]-y[1]*1) + y[0]*(x1[2]-x1[1]*1))/denom;
	
	// Calculate the maximum of the polynomial.
	return (p[0] - p[1]*p[1]/(4*p[2]));
}

double calculateP3(const short &x, unsigned short *y, double *p){
	double x1[4], x2[4], x3[4];
	for(size_t i = 0; i < 4; i++){
		x1[i] = (x+i);
		x2[i] = std::pow(x+i, 2);
		x3[i] = std::pow(x+i, 3);
	}

	double denom = (x1[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + x1[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - (x1[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + (x1[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - x1[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - (x1[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - x1[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + x1[2]*(x2[0]*x3[1]-x2[1]*x3[0]));

	p[0] = (y[0]*(x1[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + x1[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - y[1]*(x1[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + y[2]*(x1[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - x1[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - y[3]*(x1[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - x1[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + x1[2]*(x2[0]*x3[1]-x2[1]*x3[0]))) / denom;
	p[1] = ((y[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - y[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + y[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - (y[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - y[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + y[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + (y[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - y[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + y[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - (y[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - y[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + y[2]*(x2[0]*x3[1]-x2[1]*x3[0]))) / denom;
	p[2] = ((x1[1]*(y[2]*x3[3]-y[3]*x3[2]) - x1[2]*(y[1]*x3[3]-y[3]*x3[1]) + x1[3]*(y[1]*x3[2]-y[2]*x3[1])) - (x1[0]*(y[2]*x3[3]-y[3]*x3[2]) - x1[2]*(y[0]*x3[3]-y[3]*x3[0]) + x1[3]*(y[0]*x3[2]-y[2]*x3[0])) + (x1[0]*(y[1]*x3[3]-y[3]*x3[1]) - x1[1]*(y[0]*x3[3]-y[3]*x3[0]) + x1[3]*(y[0]*x3[1]-y[1]*x3[0])) - (x1[0]*(y[1]*x3[2]-y[2]*x3[1]) - x1[1]*(y[0]*x3[2]-y[2]*x3[0]) + x1[2]*(y[0]*x3[1]-y[1]*x3[0]))) / denom;
	p[3] = ((x1[1]*(x2[2]*y[3]-x2[3]*y[2]) - x1[2]*(x2[1]*y[3]-x2[3]*y[1]) + x1[3]*(x2[1]*y[2]-x2[2]*y[1])) - (x1[0]*(x2[2]*y[3]-x2[3]*y[2]) - x1[2]*(x2[0]*y[3]-x2[3]*y[0]) + x1[3]*(x2[0]*y[2]-x2[2]*y[0])) + (x1[0]*(x2[1]*y[3]-x2[3]*y[1]) - x1[1]*(x2[0]*y[3]-x2[3]*y[0]) + x1[3]*(x2[0]*y[1]-x2[1]*y[0])) - (x1[0]*(x2[1]*y[2]-x2[2]*y[1]) - x1[1]*(x2[0]*y[2]-x2[2]*y[0]) + x1[2]*(x2[0]*y[1]-x2[1]*y[0]))) / denom;

	if(p[3] == 0){
		// Handle the case of p[3] == 0.
		return (p[0] - p[1]*p[1]/(4*p[2]));
	}

	// Calculate the maximum of the polynomial.
	double xmax1 = (-2*p[2]+std::sqrt(4*p[2]*p[2]-12*p[3]*p[1]))/(6*p[3]);
	double xmax2 = (-2*p[2]-std::sqrt(4*p[2]*p[2]-12*p[3]*p[1]))/(6*p[3]);

	if((2*p[2]+6*p[3]*xmax1) < 0) // The second derivative is negative (i.e. this is a maximum).
		return (p[0] + p[1]*xmax1 + p[2]*xmax1*xmax1 + p[3]*xmax1*xmax1*xmax1);

	return (p[0] + p[1]*xmax2 + p[2]*xmax2*xmax2 + p[3]*xmax2*xmax2*xmax2);
}
