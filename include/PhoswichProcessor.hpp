#ifndef PHOSWICH_PROCESSOR_HPP
#define PHOSWICH_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class PhoswichProcessor : public Processor{
  private:
	PhoswichStructure structure;
	PhoswichWaveform waveform;

	double fast_A; /// The amplitude of the landau function for the fast pulse.
	double fast_MPV; /// The most probable value of the landau function for the fast pulse.
	double fast_Sigma; /// The sigma (roughly the width) of the landau function for the fast pulse.
	double fast_chi2; /// The chi^2/NDF for the fast pulse fit.

	double slow_A; /// The amplitude of the decaying exponential of the slow pulse.
	double slow_Slope; /// The slope of the decaying exponential of the slow pulse.
	double slow_chi2; /// The chi^2/NDF for the slow pulse fit.
	
	TF1 *fitting_func2; // Slow pulse component fitting function.

	/// Set the fit parameters for the current event.
	virtual bool SetFitParameters(ChannelEvent *event_, MapEntry *entry_);
	
	/// Fit a single trace.
	virtual bool FitPulse(TGraph *trace_, float &phase);	

	/// Process all individual events.
	virtual bool HandleEvents();
	
  public:
	PhoswichProcessor(MapFile *map_);
	
	~PhoswichProcessor();

	virtual bool Initialize(TTree *tree_);
	
	virtual void Zero();
};

#endif
