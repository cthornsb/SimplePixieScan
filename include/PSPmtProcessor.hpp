#ifndef PSPMT_PROCESSOR_HPP
#define PSPMT_PROCESSOR_HPP

#include "Processor.hpp"
#include "PSPmtMap.hpp"

class MapFile;

class PSPmtProcessor : public Processor{
  private:
	PSPmtStructure structure;
	Trace waveform;

	Plotter *tdiff_1d;
	Plotter *tof_1d;
	Plotter *long_1d;
	Plotter *xpos_1d;
	Plotter *ypos_1d;
	Plotter *ypos_xpos_2d;
	Plotter *psd_long_2d;
	Plotter *long_tof_2d;
	Plotter *loc_1d;

	std::vector<PSPmtMap> detMap;

	// Handle an individual event.
	virtual bool HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR=NULL);
	
  public:
	PSPmtProcessor(MapFile *map_);

	~PSPmtProcessor(){ }
	
	virtual void GetHists(OnlineProcessor *online_);

	/** Manually add PSPmt detector locations to the online processor detector list
	  * @param locations Vector of all PSPmt locations defined in the map
	  * @return True
	  */
	virtual bool AddDetectorLocations(std::vector<int> &locations);

	virtual void Reset();
};

#endif
