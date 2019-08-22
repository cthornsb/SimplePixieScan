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

	unsigned int count = 0;
	double prevTime = 0;
	double firstTime = 0;	
	double currTime = 0;
	double tdiff;
	
	outtree = new TTree("t", "tree");
	outtree->Branch("tdiff", &tdiff);
	outtree->Branch("time", &currTime);

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

		double time;
		unsigned int mult;
		while(getNextEntry()){
			mult = (!useTrigger ? lptr->mult : tptr->mult);
			if(mult == 0)
				continue;
			for(unsigned int j = 0; j < mult; j++){
				time = (!useTrigger ? lptr->time.at(j) : tptr->time.at(j));
				if(count++ != 0){
					currTime = time-firstTime;
					tdiff = currTime-prevTime;
					outtree->Fill();
					prevTime = currTime;
				}
				else{ firstTime = time; }
			}
		}

		std::cout << " First event time in file   = " << firstTime*8E-9 << " s.\n";
		std::cout << " Total elapsed time in file = " << currTime*8E-9 << " s.\n";

		grandTotalTime += currTime*8E-9;
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
