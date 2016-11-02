#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TH2.h"

#include "cmcalc.hpp"
#include "simpleTool.hpp"
#include "Structures.h"

class simpleComCalculator : public simpleTool {
  private:
	bool mcarlo;

  public:
	simpleComCalculator() : simpleTool(), mcarlo(false) { }
	
	void addOptions();
	
	void processArgs();
	
	int execute(int argc, char *argv[]);
};

void simpleComCalculator::addOptions(){
	addOption(optionExt("mcarlo", no_argument, NULL, 'm', "", "Read from a VANDMC monte carlo file."), userOpts, optstr);
}

void simpleComCalculator::processArgs(){
	if(userOpts.at(0).active)
		mcarlo = true;
}

int simpleComCalculator::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 1;

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 2;
	}
	
	if(!openInputFile()){
		std::cout << " Error: Failed to load input file \"" << input_filename << "\".\n";
		return 3;
	}
		
	if(!loadInputTree()){
		std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
		return 4;
	}
	
	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 5;
	}

	double ctof, energy, theta, angleCOM;
	
	TBranch *branch = NULL;
	VandleStructure *ptr = NULL;
	std::vector<double> hitTheta;

	if(!mcarlo){
		intree->SetBranchAddress("vandle", &ptr, &branch);
	}
	else{
		intree->SetMakeClass(1);
		intree->SetBranchAddress("hitTheta", &hitTheta, &branch);
	}

	if(!branch){
		std::cout << " Error: Failed to load branch \"vandle\" from input TTree.\n";
		return 6;
	}

	reaction rxn;
	rxn.SetBeam(2, 4, 7.073915);
	rxn.SetTarget(9, 19, 7.779015);
	rxn.SetRecoil(11, 22, 7.915709);
	rxn.SetEjectile(0, 1);
	rxn.SetEbeam(5.05);

	rxn.Print();

	outtree = new TTree("data", "CM Angles");
	
	if(!mcarlo){
		outtree->Branch("tof", &ctof);
		outtree->Branch("E", &energy);
	}
	outtree->Branch("lab", &theta);
	outtree->Branch("com", &angleCOM);
	
	for(unsigned int i = 0; i < intree->GetEntries(); i++){
		intree->GetEntry(i);
		
		if(!mcarlo){
			for(unsigned int j = 0; j < ptr->ctof.size(); j++){
				ctof = ptr->ctof.at(j);
				energy = ptr->energy.at(j);
				theta = ptr->theta.at(j);
		
				rxn.SetLabAngle(theta);
				angleCOM = rxn.GetEjectile()->comAngle[0];
			
				outtree->Fill();
			}
		}
		else{
			for(unsigned int j = 0; j < hitTheta.size(); j++){
				theta = hitTheta.at(j);
				rxn.SetLabAngle(theta);
				angleCOM = rxn.GetEjectile()->comAngle[0];
				
				outtree->Fill();
			}
		}
	}
	
	outfile->cd();
	outtree->Write();
	
	std::cout << "\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
	
	return 0;
}

int main(int argc, char *argv[]){
	simpleComCalculator obj;
	
	return obj.execute(argc, argv);
}
