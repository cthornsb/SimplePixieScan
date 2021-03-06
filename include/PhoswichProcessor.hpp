#ifndef PHOSWICH_PROCESSOR_HPP
#define PHOSWICH_PROCESSOR_HPP

#include "Processor.hpp"

class MapFile;

class PhoswichProcessor : public Processor{
  private:
	PhoswichStructure structure;
	Trace waveform;
  
	double fast_A; /// The amplitude of the landau function for the fast pulse.
	double fast_MPV; /// The most probable value of the landau function for the fast pulse.
	double fast_Sigma; /// The sigma (roughly the width) of the landau function for the fast pulse.
	double fast_chi2; /// The chi^2/NDF for the fast pulse fit.
	float fast_qdc; /// The integral of the fast portion of the pulse.
	unsigned int fast_x1; /// The lower limit of the fitting range for the fast pulse.
	unsigned int fast_x2; /// The upper limit of the fitting range for the fast pulse.

	float slow_qdc; /// The integral of the slow portion of the pulse.
	int fitting_low2;
	int fitting_high2;

	Plotter *fast_energy_1d;
	Plotter *slow_energy_1d;
	Plotter *energy_2d;
	Plotter *phase_1d;

	/// Set the fit parameters for the current event.
	//virtual bool SetFitParameters(ChanEvent *event_, MapEntry *entry_);
	
	/// Fit a single trace.
	//virtual bool FitPulse(TGraph *trace_, float &phase);	

	/// Set the CFD parameters for the current event.
	virtual bool SetCfdParameters(ChanEvent *event_, MapEntry *entry_);

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PhoswichProcessor(MapFile *map_);
	
	~PhoswichProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);
};

#endif
