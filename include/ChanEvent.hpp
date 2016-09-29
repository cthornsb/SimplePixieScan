#ifndef CHANEVENT_HPP
#define CHANEVENT_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>

#include "XiaData.hpp"

class ChanEvent : public XiaData {
public:
    bool valid_chan; /// True if the high resolution energy and time are valid.
    bool ignore; /// Ignore this event.
    
    float phase; /// Phase (leading edge) of trace (in ADC clock ticks (4E-9 Hz for 250 MHz digitizer)).
    float baseline; /// The baseline of the trace.
    float stddev; /// Standard deviation of the baseline.
    float maximum; /// The baseline corrected maximum value of the trace.
    float qdc; /// The calculated (baseline corrected) qdc.
    size_t max_index; /// The index of the maximum trace bin (in ADC clock ticks).
    
    float *cfdvals; ///
    
    /// Default constructor.
    ChanEvent();
    
    /// Constructor from a XiaData. ChannelEvent will take ownership of the XiaData.
    ChanEvent(XiaData *event_);
    
    /// Destructor.
    ~ChanEvent();
    
    /// Calculate the trace baseline, baseline standard deviation, and find the pulse maximum.
    float ComputeBaseline();
    
    /// Integrate the baseline corrected trace in the range [start_, stop_] and return the result.
    float IntegratePulse(const size_t &start_=0, const size_t &stop_=0);
    
    /// Perform CFD analysis on the waveform.
    float AnalyzeCFD(const float &F_=0.5, const size_t &D_=1, const size_t &L_=1);
    
    /// Clear all variables and clear the trace vector and arrays.
    void Clear();
};

#endif
