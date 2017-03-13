#include <iostream>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "simpleTool.hpp"
#include "Structures.h"

class instantTime : public simpleTool {
  public:
	instantTime() : simpleTool() { }
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void instantTime::addOptions(){
}

bool instantTime::processArgs(){
	return true;
}

int instantTime::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;	

	if(input_filename.empty()){
		std::cout << " Error: Must specify input filename!\n";
		return 1;
	}

	std::cout << " Processing " << input_filename << ".\n";
	if(!openInputFile()) return false;

	if(!loadInputTree()){
		std::cout << "  Failed to load input tree!\n";
		return 2;
	}

	TBranch *branch = NULL;
	LogicStructure *ptr = NULL;

	intree->SetBranchAddress("logic", &ptr, &branch);

	if(!branch){
		std::cout << " Error: Failed to load branch \"trigger\" from input TTree.\n";
		return false;
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
	double prevTime;
	double currTime;
	
	double tdiff;
	
	outtree = new TTree("t", "tree");
	outtree->Branch("tdiff", &tdiff);
	outtree->Branch("time", &currTime);

	std::cout << " Processing " << intree->GetEntries() << " entries.\n";
	for(int i = 0; i < intree->GetEntries(); i++){
		intree->GetEntry(i);
		if(ptr->mult == 0)
			continue;
		for(unsigned int j = 0; j < ptr->mult; j++){
			if(count != 0){
				currTime = ptr->time.at(j);
				tdiff = currTime-prevTime;
				outtree->Fill();
				prevTime = currTime;
			}
			else{
				prevTime = ptr->time.at(j);
			}
			count++;
		}
	}

	outfile->cd();
	outtree->Write();
	
	return 0;
}

int main(int argc, char *argv[]){
	instantTime obj;
	
	return obj.execute(argc, argv);
}
