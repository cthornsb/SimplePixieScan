#include <cmath>

#include "ChanEvent.hpp"

/////////////////////////////////////////////////////////////////////
// ChanEvent
/////////////////////////////////////////////////////////////////////

/// Default constructor.
ChanEvent::ChanEvent(){
	Clear();
}

/// Constructor from a XiaData. ChanEvent will take ownership of the XiaData.
ChanEvent::ChanEvent(XiaData *event_) : XiaData(event_) {
	Clear();
}

ChanEvent::~ChanEvent(){
	if(cfdvals) delete[] cfdvals;
}

float ChanEvent::ComputeBaseline(){
	if(adcTrace.empty()){ return -9999; }
	if(baseline > 0){ return baseline; }

	// Find the baseline.
	baseline = 0.0;
	size_t sample_size = (10 <= adcTrace.size() ? 10:adcTrace.size());
	for(size_t i = 0; i < sample_size; i++){
		baseline += (float)adcTrace[i];
	}
	baseline = baseline/sample_size;
	
	// Calculate the standard deviation.
	stddev = 0.0;
	for(size_t i = 0; i < sample_size; i++){
		stddev += ((float)adcTrace[i] - baseline)*((float)adcTrace[i] - baseline);
	}
	stddev = std::sqrt((1.0/sample_size) * stddev);
	
	// Find the maximum value and the maximum bin.
	maximum = -9999.0;
	for(size_t i = 0; i < adcTrace.size(); i++){
		if(adcTrace[i]-baseline > maximum){ 
			maximum = adcTrace[i]-baseline;
			max_index = i;
		}
	}
	
	return baseline;
}

float ChanEvent::IntegratePulse(const size_t &start_/*=0*/, const size_t &stop_/*=0*/){
	if(adcTrace.empty() || baseline < 0.0){ return -9999; }
	
	size_t stop = (stop_ == 0?adcTrace.size():stop_);
	
	qdc = 0.0;
	for(size_t i = start_+1; i < stop; i++){ // Integrate using trapezoidal rule.
		qdc += 0.5*(adcTrace[i-1] + adcTrace[i]) - baseline;
	}

	return qdc;
}

/// Perform CFD analysis on the waveform.
float ChanEvent::AnalyzeCFD(const float &F_/*=0.5*/, const size_t &D_/*=1*/, const size_t &L_/*=1*/){
	if(adcTrace.empty() || baseline < 0){ return -9999; }
	if(!cfdvals)
		cfdvals = new float[adcTrace.size()];
	
	float cfdMinimum = 9999;
	size_t cfdMinIndex = 0;
	
	phase = -9999;

	// Compute the cfd waveform.
	for(size_t cfdIndex = 0; cfdIndex < adcTrace.size(); ++cfdIndex){
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

void ChanEvent::Clear(){
	phase = -9999;
	maximum = -9999;
	baseline = -9999;
	stddev = -9999;
	qdc = -9999;
	max_index = 0;

	valid_chan = false;
	ignore = false;
	
	cfdvals = NULL;
	
	if(cfdvals) 
		delete[] cfdvals;
}
