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
	bool inverse;
	
	std::string configFilename;

  public:
	simpleComCalculator() : simpleTool(), mcarlo(false), inverse(false), configFilename("") { }
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void simpleComCalculator::addOptions(){
	addOption(optionExt("mcarlo", no_argument, NULL, 'm', "", "Read from a VANDMC monte carlo file."), userOpts, optstr);
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read reaction information from an input file."), userOpts, optstr);
}

bool simpleComCalculator::processArgs(){
	if(userOpts.at(0).active)
		mcarlo = true;
	if(userOpts.at(1).active)
		configFilename = userOpts.at(1).argument;

	return true;
}

int simpleComCalculator::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 2;
	}

	std::string userInput;
	reaction rxn;
	if(!configFilename.empty()){
		std::ifstream configFile(configFilename.c_str());
		if(!configFile.good()){
			std::cout << " Error: Failed to load input configuration file.\n";
			return 3;
		}
		float Z, A, BEA, state;
		configFile >> Z >> A >> BEA;
		rxn.SetBeam(Z, A, BEA);
		configFile >> Z >> A >> BEA;
		rxn.SetTarget(Z, A, BEA);
		configFile >> Z >> A >> BEA >> state;
		rxn.SetRecoil(Z, A, BEA);
		rxn.SetRecoilEx(state);
		configFile >> Z >> A >> BEA >> state;
		rxn.SetEjectile(Z, A, BEA);
		rxn.SetEjectileEx(state);
		configFile >> state;
		rxn.SetEbeam(state);
	}
	else{
		// Save the configuration file.
		bool saveToFile = false;
		std::ofstream outConfig;
		std::cout << " Manual reaction configuration mode.\n";
		std::cout << " Save when finished? (y/n) "; std::cin >> userInput;
		if(userInput == "y" || userInput == "Y" || userInput == "yes"){
			std::cout << " Enter config filename: "; std::cin >> userInput;
			outConfig.open(userInput.c_str());
			saveToFile = true;
		}
	
		float Z, A, BEA, state;
		std::cout << " Enter beam Z, A, and BE/A: "; std::cin >> Z >> A >> BEA;
		if(saveToFile) outConfig << Z << "\t" << A << "\t" << BEA << "\n";
		rxn.SetBeam(Z, A, BEA);
		std::cout << " Enter target Z, A, and BE/A: "; std::cin >> Z >> A >> BEA;
		if(saveToFile) outConfig << Z << "\t" << A << "\t" << BEA << "\n";
		rxn.SetTarget(Z, A, BEA);
		std::cout << " Enter recoil Z, A, BE/A, and excitation: "; std::cin >> Z >> A >> BEA >> state;
		if(saveToFile) outConfig << Z << "\t" << A << "\t" << BEA << "\t" << state << "\n";
		rxn.SetRecoil(Z, A, BEA);
		rxn.SetRecoilEx(state);
		std::cout << " Enter ejectile Z, A, BE/A, and excitation: "; std::cin >> Z >> A >> BEA >> state;
		if(saveToFile) outConfig << Z << "\t" << A << "\t" << BEA << "\t" << state << "\n";
		rxn.SetEjectile(Z, A, BEA);
		rxn.SetEjectileEx(state);
		std::cout << " Enter beam energy: "; std::cin >> state;
		if(saveToFile) outConfig << state << "\n";
		rxn.SetEbeam(state);
		std::cout << std::endl;
		outConfig.close();
	}

	// Make sure all settings are correct.
	rxn.Print();
	std::cout << "\n  Correct? (y/n) "; std::cin >> userInput;
	if(!(userInput == "y" || userInput == "Y" || userInput == "yes"))
		return 3;

	if(!rxn.IsAboveThreshold()){
		std::cout << " Error: Energy is below reaction threshold by " << (rxn.GetThresholdEnergy() - rxn.GetBeamEnergy()) << " MeV.\n";
		return 4;
	}

	if(!rxn.IsNormalKinematics())
		inverse = true;

	if(!openInputFile()){
		std::cout << " Error: Failed to load input file \"" << input_filename << "\".\n";
		return 5;
	}
		
	if(!loadInputTree()){
		std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
		return 6;
	}
	
	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 7;
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
		return 8;
	}

	outtree = new TTree("data", "CM Angles");
	
	if(!mcarlo){
		outtree->Branch("tof", &ctof);
		outtree->Branch("E", &energy);
	}
	outtree->Branch("lab", &theta);
	outtree->Branch("com", &angleCOM);
	
	TH2D *h2d = new TH2D("h2d", "CoM Angle vs. ctof", 500, -10, 100, 36, 0, 180);
	
	progressBar pbar;
	pbar.start(intree->GetEntries());

	unsigned int badCount = 0;
	for(unsigned int i = 0; i < intree->GetEntries(); i++){
		pbar.check(i);

		intree->GetEntry(i);
		
		if(!mcarlo){
			for(unsigned int j = 0; j < ptr->ctof.size(); j++){
				if(ptr->r.at(j) >= 0.65){
					badCount++;
					continue;
				}
		
				ctof = ptr->ctof.at(j);
				energy = ptr->energy.at(j);
				theta = ptr->theta.at(j);
		
				rxn.SetLabAngle(theta);
				if(!inverse) angleCOM = rxn.GetEjectile()->comAngle[0];
				else angleCOM = 180 - rxn.GetEjectile()->comAngle[0];
			
				outtree->Fill();
				h2d->Fill(ctof, angleCOM);
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
	h2d->Write();
	
	std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
	std::cout << "  Rejected " << badCount << " events.\n";
	
	return 0;
}

int main(int argc, char *argv[]){
	simpleComCalculator obj;
	
	return obj.execute(argc, argv);
}
