#ifndef GENERIC_PROCESSOR_HPP
#define GENERIC_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;
class Plotter;

class GenericProcessor : public Processor{
  private:
	GenericStructure structure;
	GenericWaveform waveform;
  
	Plotter *loc_tdiff_2d;
	Plotter *loc_energy_2d;
	Plotter *loc_phase_2d;
	Plotter *loc_1d;
  
	virtual bool HandleEvents();
	
  public:
	GenericProcessor(MapFile *map_);

	~GenericProcessor();

	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
