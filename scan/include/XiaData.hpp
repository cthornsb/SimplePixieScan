#ifndef XIADATA_HPP
#define XIADATA_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>

/*! \brief A pixie16 channel event
 *
 * All data is grouped together into channels.  For each pixie16 channel that
 * fires the energy, time (both trigger time and event time), and trace (if
 * applicable) are obtained.  Additional information includes the channels
 * identifier, calibrated energies, trace analysis information.
 * Note that this currently stores raw values internally through pixie word types
 *   but returns data values through native C types. This is potentially non-portable.
 */
class XiaData{
  public:
	unsigned short energy; /// Raw pixie energy.
	double time; /// Raw pixie event time. Measured in filter clock ticks (8E-9 Hz for RevF).
	
	size_t traceLength;
	unsigned short *adcTrace; /// ADC trace capture.
	
	size_t numQdcs; /// Number of QDCs onboard.
	unsigned int *qdcValue; /// QDCs from onboard.
	
	unsigned short headerLength; /// Length of the pixie header in words.
	unsigned short eventLength; /// Length of the total event in words.
	
	unsigned short crateNum; /// Crate number.
	unsigned short slotNum; ///Slot number (not the same as the module number).
	unsigned short modNum; /// Module number (not the same as the slot number).
	unsigned short chanNum; /// Channel number.
	unsigned short cfdTime; /// CFD trigger time in units of 1/256 pixie clock ticks.
	unsigned int eventTimeLo; /// Lower 32 bits of pixie16 event time.
	unsigned int eventTimeHi; /// Upper 32 bits of pixie16 event time.
	
	bool virtualChannel; /// Flagged if generated virtually in Pixie DSP.
	bool pileupBit; /// Pile-up flag from Pixie.
	bool saturatedBit; /// Saturation flag from Pixie.
	bool cfdForceTrig; /// CFD was forced to trigger.
	bool cfdTrigSource; /// The ADC that the CFD/FPGA synched with.
	bool outOfRange; /// Set to true if the trace is saturated.
	bool saturatedTrace; /// Set to true if the ADC trace is saturated.
	
	/// Default constructor.
	XiaData();
	
	/// Constructor from a pointer to another XiaData.
	XiaData(XiaData *other_);
	
	/// Virtual destructor.
	virtual ~XiaData();
	
	/// Get the event ID number (mod * chan).
	int getID(){ return(modNum*16+chanNum); }
	
	/// Fill the trace by reading from a character array.
	void copyTrace(char *ptr_, const unsigned short &size_);

	/// Fill the QDC array by reading a character array.
	void copyQDCs(char *ptr_, const unsigned short &size_);

	/// Return true if the time of arrival for rhs is later than that of lhs.
	static bool compareTime(XiaData *lhs, XiaData *rhs){ return (lhs->time < rhs->time); }
	
	/// Return true if lhs has a lower event id (mod*16 + chan) than rhs.
	static bool compareChannel(XiaData *lhs, XiaData *rhs){ return ((lhs->modNum*16+lhs->chanNum) < (rhs->modNum*16+rhs->chanNum)); }
	
	/// Return one of the onboard qdc values.
	unsigned int getQdcValue(const size_t &id){ return (id < 0 || id >= numQdcs ? -1 : qdcValue[id]); }
	
	/// Clear all variables.
	void clear();
	
	/// Delete the trace.
	void clearTrace();
	
	/// Delete the QDC array.
	void clearQDCs();
	
	/// Print event information to the screen.
	void print();
	
	/// Print additional information to the screen.
	virtual void print2(){ }

	/** Responsible for decoding individual pixie events from a binary input file.
	  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
	  * \param[in]  module	  The current module number being scanned.
	  * \param[out] bufferIndex The current index in the module buffer.
	  * \return Only false currently. This method is only a stub.
	  */
	bool readEventRevD(unsigned int *buf, unsigned int &bufferIndex, unsigned int module=9999);

	/** Responsible for decoding individual pixie events from a binary input file.
	  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
	  * \param[in]  module	  The current module number being scanned.
	  * \param[out] bufferIndex The current index in the module buffer.
	  * \return True if the event was successfully read, or false otherwise.
	  */
	bool readEventRevF(unsigned int *buf, unsigned int &bufferIndex, unsigned int module=9999);

	/// Get the size of the XiaData event when written to disk by ::writeEventRevF (in 4-byte words).
	size_t getEventLengthRevF();
	
	/** Write a pixie style event to a binary output file. Output data may
	  * be written to both an ofstream and a character array. One of the
	  * pointers must not be NULL.
	  * 
	  * \param[in] file_ Pointer to an ofstream output binary file.
	  * \param[in] array_ Pointer to a character array into which data will be written.
	  * \return The number of bytes written to the file upon success and -1 otherwise.
	  */
	int writeEventRevF(std::ofstream *file_, char *array_);
	
	/** Responsible for decoding events with arbitrary formatting from a binary input file.
	  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
	  * \param[in]  modNum	 The current module number being scanned.
	  * \param[out] bufferIndex The current index in the module buffer.
	  * \return False by default.
	  */
	virtual bool readEvent(unsigned int *buf, unsigned int &bufferIndex){ return false; }

	/** Write an arbitrary event to a binary output file. Output data may
	  * be written to both an ofstream and a character array. One of the
	  * pointers must not be NULL.
	  * 
	  * \param[in] file_ Pointer to an ofstream output binary file.
	  * \param[in] array_ Pointer to a character array into which data will be written.
	  * \return -1 by default.
	  */
	virtual int writeEvent(std::ofstream *file_, char *array_){ return -1; }
};

class ChannelEvent : public XiaData {
  public:
	bool valid_chan; /// True if the high resolution energy and time are valid.
	bool ignore; /// Ignore this event.
	
	double hiresTime; /// High resolution time obtained from the trigger time and the trace phase.
	
	float phase; /// Phase (leading edge) of trace (in ADC clock ticks (4E-9 Hz for 250 MHz digitizer)).
	float baseline; /// The baseline of the trace.
	float stddev; /// Standard deviation of the baseline.
	float maximum; /// The baseline corrected maximum value of the trace.
	float qdc; /// The calculated (baseline corrected) qdc.
	float qdc2; /// An additional qdc value.
	
	unsigned short max_index; /// The index of the maximum trace bin (in ADC clock ticks).
	unsigned short max_ADC; /// The uncorrected maximum ADC value of the trace.
	unsigned short cfdIndex; /// The index in the trace just above the CFD threshold.
	
	double cfdPar[7]; /// Array of floats for storing cfd polynomial fits.

	float *cfdvals; ///
	
	/// Default constructor.
	ChannelEvent();
	
	/// Constructor from a XiaData. ChannelEvent will take ownership of the XiaData.
	ChannelEvent(XiaData *event_);
	
	/// Destructor.
	~ChannelEvent();
	
	/// Calculate the trace baseline, baseline standard deviation, and find the pulse maximum.
	float ComputeBaseline();
	
	/// Integrate the baseline corrected trace in the range [start_, stop_] and return the result.
	float IntegratePulse(const size_t &start_=0, const size_t &stop_=0, bool calcQdc2=false);

	/// Perform traditional CFD analysis on the waveform.
	float AnalyzeCFD(const float &F_=0.5, const size_t &D_=1, const size_t &L_=1);
	
	/// Perform polynomial CFD analysis on the waveform.
	float AnalyzePolyCFD(const float &F_=0.5);
	
	/// Clear all variables and clear the trace vector and arrays.
	void Clear();
	
	/** Responsible for decoding ChannelEvents from a binary input file.
	  * \param[in]  buf	     Pointer to an array of unsigned ints containing raw event data.
	  * \param[in]  modNum	 The current module number being scanned.
	  * \param[out] bufferIndex The current index in the module buffer.
	  * \return True if the event was successfully read, or false otherwise.
	  */
	bool readEvent(unsigned int *buf, unsigned int &bufferIndex);

	/** Write a ChannelEvent to a binary output file. Output data may
	  * be written to both an ofstream and a character array. One of the
	  * pointers must not be NULL.
	  * 
	  * \param[in] file_ Pointer to an ofstream output binary file.
	  * \param[in] array_ Pointer to a character array into which data will be written.
	  * \param[in] recordTrace_ If set to true, the ADC trace will be written to output.
	  * \return The number of bytes written to the file upon success and -1 otherwise.
	  */
	int writeEvent(std::ofstream *file_, char *array_, bool recordTrace_=false);
	
	/// Print additional information to the screen.
	virtual void print2();
};

double calculateP2(const short &x0, unsigned short *y, double *p);

double calculateP3(const short &x0, unsigned short *y, double *p);

#endif
