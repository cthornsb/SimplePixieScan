#ifndef NONWICH_PROCESSOR_HPP
#define NONWICH_PROCESSOR_HPP

#include "Processor.hpp"

#include "Structures.h"

class MapFile;

class NonwichProcessor : public Processor{
  private:
	NonwichStructure structure;
	NonwichWaveform waveform;  
  
	Plotter *dE_energy_1d;
	Plotter *E_energy_1d;
	Plotter *tdiff_1d;
	Plotter *energy_2d;
	Plotter *dE_phase_1d;
	Plotter *E_phase_1d;
  
	virtual bool HandleEvents();
	
  public:
	NonwichProcessor(MapFile *map_);

	~NonwichProcessor();
	
	virtual void GetHists(std::vector<Plotter*> &plots_);
};

#endif
