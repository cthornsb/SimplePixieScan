#include <iostream>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "simpleTool.hpp"
#include "Structures.hpp"

class instantTime : public simpleTool {
  public:
	instantTime() : simpleTool(), useTrigger(false) { }
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);

  private:
	bool useTrigger;
};

void instantTime::addOptions(){
	addOption(optionExt("trigger", no_argument, NULL, 'T', "", "Read time from \"trigger\" branch instead."), userOpts, optstr);
}

bool instantTime::processArgs(){
	if(userOpts.at(0).active)
		useTrigger = true;
	return true;
}

int instantTime::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;	

	if(input_filename.empty()){
		std::cout << " Error: Must specify input filename!\n";
		return 1;
	}

	if(output_filename.empty()){
		std::cout << " Warning: Output filename not specified. Setting output filename to \"instantaneous.root\".\n";
		output_filename = "instantaneous.root";
	}

	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 3;
	}

	unsigned int count;

	double prevTime = 0;
	double firstTime = 0;
	double currTime = 0;

	double time;
	double tdiff;
	
	outtree = new TTree("data", "tree");
	outtree->Branch("tdiff", &tdiff);
	outtree->Branch("time", &time);

	double timeOffset = 0;
	double grandTotalTime = 0;

	while(openInputFile()){
		std::cout << " Processing " << input_filename << ".\n";
		
		if(!loadInputTree()){
			std::cout << "  Failed to load input tree!\n";
			return 2;
		}

		std::string bname = (!useTrigger ? "logic" : "trigger");

		TBranch *branch = NULL;
		LogicStructure *lptr = NULL;
		TriggerStructure *tptr = NULL;

		// Set the branch address
		if(!useTrigger)
			intree->SetBranchAddress("logic", &lptr, &branch);
		else
			intree->SetBranchAddress("trigger", &tptr, &branch);
			
		if(!branch){
			std::cout << " Error: Failed to load branch \"" << bname << "\" from input TTree.\n";
			return false;
		}

		count = 0;
		unsigned int mult;
		while(getNextEntry()){
			mult = (!useTrigger ? lptr->mult : tptr->mult);
			for(unsigned int j = 0; j < mult; j++){
				currTime = (!useTrigger ? lptr->time.at(j) : tptr->time.at(j));
				if(count++ == 0){ // Get the time of the first event
					firstTime = currTime;
					prevTime = currTime;
				}
				
				// Compute output variables
				time = (currTime - firstTime + timeOffset)*8E-9; // Now in seconds
				tdiff = (currTime - prevTime)*8E-9; // Now in seconds
				prevTime = currTime;	

				if(tdiff < 0) // Check for negative time difference
					std::cout << " Warning! Negative time jump encountered (tdiff=" << tdiff << ")!!!\n";
	
				// Fill the tree
				outtree->Fill();
			}
		}

		std::cout << " First event time in file   = " << firstTime*8E-9 << " s.\n";
		std::cout << " Total elapsed time in file = " << (prevTime-firstTime)*8E-9 << " s.\n";

		grandTotalTime += (prevTime-firstTime)*8E-9;

		// Update the time offset between files
		timeOffset += (prevTime-firstTime);
	}

	outfile->cd();
	outtree->Write();
	
	std::cout << " GRAND TOTAL Time = " << grandTotalTime << " s.\n";

	return 0;
}

int main(int argc, char *argv[]){
	instantTime obj;
	
	return obj.execute(argc, argv);
}
