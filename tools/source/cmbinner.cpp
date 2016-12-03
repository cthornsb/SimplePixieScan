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
	double startAngle;
	double stopAngle;
	double binWidth;
	int nBins;

	double *xbins;

	bool mcarlo;
	bool printMode;
	bool defaultMode;
	bool treeMode;
	
	std::string configFilename;

	reaction rxn;

	bool setupReaction();

	void setAngles();

  public:
	simpleComCalculator() : simpleTool(), startAngle(0), stopAngle(180), binWidth(1), nBins(180), xbins(NULL), mcarlo(false), printMode(false), defaultMode(false), treeMode(false), configFilename("") { }

	~simpleComCalculator();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

bool simpleComCalculator::setupReaction(){
	std::string userInput;

	// Read the reaction parameters from the config file.
	bool setupComplete;
	if(!configFilename.empty())
		setupComplete = rxn.Read(configFilename.c_str());
	else
		setupComplete = rxn.Read();

	if(!setupComplete){			
		std::cout << " Error: Failed to setup reaction parameters!\n";
		return false;
	}

	// Make sure all settings are correct.
	rxn.Print();
	std::cout << "\n  Correct? (y/n) "; std::cin >> userInput;
	if(!(userInput == "y" || userInput == "Y" || userInput == "yes"))
		return false;

	if(!rxn.IsAboveThreshold()){
		std::cout << " Error: Energy is below reaction threshold by " << (rxn.GetThresholdEnergy() - rxn.GetBeamEnergy()) << " MeV.\n";
		return false;
	}

	return true;
}

void simpleComCalculator::setAngles(){
	// Set starting angle.
	while(true){
		std::cout << " Enter starting CoM angle: "; std::cin >> startAngle;
		if(startAngle < 0){
			std::cout << "  Error: Illegal starting angle (" << startAngle << "). Value may not be negative.\n";
			continue;
		}
		break;
	}

	// Set stopping angle.
	while(true){
		std::cout << " Enter stopping CoM angle: "; std::cin >> stopAngle;
		if(stopAngle < 0){
			std::cout << "  Error: Illegal stopping angle (" << stopAngle << "). Value may not be negative.\n";
			continue;
		}
		else if(stopAngle <= startAngle){
			std::cout << " Error: Illegal stop angle (" << stopAngle << "). Value must be greater than " << startAngle << ".\n";
		}
		break;
	}

	// Set angular bin width.
	while(true){
		std::cout << " Enter angular bin width: "; std::cin >> binWidth;
		if(stopAngle < 0 || stopAngle > 180){
			std::cout << "  Error: Illegal bin width (" << binWidth << "). Value must be between 0 and 180.\n";
			continue;
		}
		break;
	}

	// Calculate the number of bins.
	nBins = (int)((stopAngle - startAngle)/binWidth);

	std::cout << std::endl;
}

simpleComCalculator::~simpleComCalculator(){
	if(xbins) delete[] xbins;
}

void simpleComCalculator::addOptions(){
	addOption(optionExt("mcarlo", no_argument, NULL, 'm', "", "Read from a VANDMC monte carlo file."), userOpts, optstr);
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read reaction information from an input file."), userOpts, optstr);
	addOption(optionExt("tree", no_argument, NULL, 't', "", "Fill a TTree with reaction data."), userOpts, optstr);
	addOption(optionExt("print", no_argument, NULL, 'p', "", "Instead of scanning a file, print out kinematics information."), userOpts, optstr);
	addOption(optionExt("default", no_argument, NULL, 'd', "", "When using \"--print\" or \"--tree\" mode, use default settings of theta=[0,180] w/ 1 degree steps."), userOpts, optstr);
}

bool simpleComCalculator::processArgs(){
	if(userOpts.at(0).active)
		mcarlo = true;
	if(userOpts.at(1).active)
		configFilename = userOpts.at(1).argument;
	if(userOpts.at(2).active)
		treeMode = true;
	if(userOpts.at(3).active)
		printMode = true;
	if(userOpts.at(4).active)
		defaultMode = true;

	return true;
}

int simpleComCalculator::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(!printMode){ // Standard operation.
		if(input_filename.empty()){
			std::cout << " Error: Input filename not specified!\n";
			return 1;
		}

		if(output_filename.empty()){
			std::cout << " Error: Output filename not specified!\n";
			return 2;
		}

		if(!setupReaction())
			return 3;

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

		if(treeMode || mcarlo){
			outtree = new TTree("data", "CM Angles");
	
			if(!mcarlo){
				outtree->Branch("tof", &ctof);
				outtree->Branch("E", &energy);
			}
			outtree->Branch("lab", &theta);
			outtree->Branch("com", &angleCOM);
		}

		TH2D *h2d = NULL;
		if(!mcarlo){
			std::cout << std::endl;

			if(!defaultMode) setAngles();

			xbins = new double[nBins+1];

			particle *ejectile = rxn.GetEjectile();

			double comAngle = startAngle;
			for(int i = 0; i <= nBins; i++){
				if(!ejectile->inverseKin){
					rxn.SetComAngle(comAngle);
					xbins[i] = ejectile->labAngle[0];				
				}
				else{
					rxn.SetComAngle(180-comAngle);
					xbins[nBins-i] = ejectile->labAngle[0];
				}

				comAngle += binWidth;
			}

			if(!ejectile->inverseKin){
				comAngle = startAngle;
				std::cout << "bin\tlowCom\thighCom\tlowLab\thighLab\n";
				for(int i = 0; i < nBins; i++){
					std::cout << i << "\t" << comAngle << "\t" << (comAngle + binWidth) << "\t" << xbins[i] << "\t" << xbins[i+1] << "\n";
					comAngle += binWidth;			
				}
			}
			else{
				comAngle = stopAngle;
				std::cout << "bin\thighCom\tlowCom\tlowLab\thighLab\n";
				for(int i = 0; i < nBins; i++){
					std::cout << i << "\t" << comAngle << "\t" << (comAngle - binWidth) << "\t" << xbins[i] << "\t" << xbins[i+1] << "\n";
					comAngle -= binWidth;			
				}
			}

			int nBinsY = 525;
			double yLow = -10;
			double yHigh = 100;

			h2d = new TH2D("h2d", "Lab Angle vs. ctof", nBins, xbins, nBinsY, yLow, yHigh);
			h2d->GetXaxis()->SetTitle("Lab Angle (deg)");
			h2d->GetYaxis()->SetTitle("Neutron ToF (ns)");
			h2d->GetZaxis()->SetTitle("Counts per 0.2 ns");
		}

		progressBar pbar;
		pbar.start(intree->GetEntries());

		unsigned int badCount = 0;
		for(unsigned int i = 0; i < intree->GetEntries(); i++){
			pbar.check(i);

			intree->GetEntry(i);
		
			if(!mcarlo){
				for(unsigned int j = 0; j < ptr->mult; j++){
					if(ptr->r.at(j) >= 0.65){
						badCount++;
						continue;
					}
		
					if(treeMode){
						ctof = ptr->ctof.at(j);
						energy = ptr->energy.at(j);
						theta = ptr->theta.at(j);
		
						rxn.SetLabAngle(theta);
						angleCOM = rxn.GetEjectile()->comAngle[0];
			
						outtree->Fill();
					}
					h2d->Fill(ptr->theta.at(j), ptr->ctof.at(j));
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
		if(treeMode || mcarlo)
			outtree->Write();
		if(!mcarlo)
			h2d->Write();

		if(treeMode || mcarlo) std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
		if(!mcarlo) std::cout << "\n\n Done! Wrote " << h2d->GetEntries() << " entries to histogram.\n";
		std::cout << "  Rejected " << badCount << " events.\n";
	}
	else{ // Print kinematics to the screen instead of scanning an input file.
		if(!setupReaction())
			return 3;

		std::cout << std::endl;

		if(!defaultMode) setAngles();

		// Print some information to the screen.
		std::cout << " Using CoM angular range [" << startAngle << ", " << startAngle+nBins*binWidth << "] and " << nBins << " bins.\n\n";
		std::cout << "lowCom\tlowLab\tlowE\tlowV\tmidCom\tmidLab\tmidE\tmidV\thighCom\thighLab\thighE\thighV\n";

		particle *ejectile = rxn.GetEjectile();

		double comAngle = startAngle;
		for(int i = 0; i < nBins; i++){
			if(!ejectile->inverseKin){
				if(i == 0){
					rxn.SetComAngle(comAngle);
					std::cout << ejectile->print(false);
				}
				else std::cout << ejectile->print(false);
				rxn.SetComAngle(comAngle + binWidth/2.0);
				std::cout << "\t" << ejectile->print(false);
				rxn.SetComAngle(comAngle + binWidth);
				std::cout << "\t" << ejectile->print(false) << std::endl;
			}
			else{
				if(i == 0){
					rxn.SetComAngle(180-comAngle);
					std::cout << ejectile->print(false);
				}
				else std::cout << ejectile->print(false);
				rxn.SetComAngle(180 - comAngle - binWidth/2.0);
				std::cout << "\t" << ejectile->print(false);
				rxn.SetComAngle(180 - comAngle - binWidth);
				std::cout << "\t" << ejectile->print(false) << std::endl;
			}

			comAngle += binWidth;
		}	
	}
	
	return 0;
}

int main(int argc, char *argv[]){
	simpleComCalculator obj;
	
	return obj.execute(argc, argv);
}
