#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TCutG.h"

#include "cmcalc.hpp"
#include "simpleTool.hpp"
#include "Structures.h"

class simpleComCalculator : public simpleTool {
  private:
	double startAngle;
	double stopAngle;
	double binWidth;
	double threshold;
	double userEnergy;
	int nBins;

	double *xbins;

	bool mcarlo;
	bool printMode;
	bool defaultMode;
	bool treeMode;
	
	TCutG *cut;

	std::string cutFilename;
	std::string configFilename;

	reaction rxn;

	bool setupReaction();

	void setAngles();

  public:
	simpleComCalculator() : simpleTool(), startAngle(0), stopAngle(180), binWidth(1), threshold(-1), userEnergy(-1), nBins(180), xbins(NULL), mcarlo(false), printMode(false), defaultMode(false), treeMode(false), cut(NULL), cutFilename(""), configFilename("") { }

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

	// Check that the kinematics setup completed successfully.
	if(!setupComplete){			
		std::cout << " Error: Failed to setup reaction parameters!\n";
		return false;
	}

	// Set the beam energy to the user defined value.
	if(userEnergy > 0) rxn.SetEbeam(userEnergy);

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
	addOption(optionExt("threshold", required_argument, NULL, 'T', "<threshold>", "Use a software threshold on the trqce QDC (not used by default)."), userOpts, optstr);
	addOption(optionExt("energy", required_argument, NULL, 'E', "<energy>", "Specify the beam energy in MeV."), userOpts, optstr);
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
	if(userOpts.at(5).active)
		threshold = strtod(userOpts.at(5).argument.c_str(), 0);
	if(userOpts.at(6).active)
		userEnergy = strtod(userOpts.at(6).argument.c_str(), 0);

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

		if(!openOutputFile()){
			std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
			return 4;
		}

		// Load a root TCutG to use as a gate.
		bool useTCutG = false;
		if(!cut_filename.empty()){
			if(!loadTCutG()){
				std::cout << " Error: Failed to load input TCutG \"" << cut_filename << "\".\n";
				return 8;
			}
			useTCutG = true;
		}

		double ctof, energy, tqdc, theta, angleCOM;
		int location;

		if(treeMode || mcarlo){
			outtree = new TTree("data", "CM Angles");

			if(!mcarlo){
				outtree->Branch("tof", &ctof);
				outtree->Branch("E", &energy);
				outtree->Branch("tqdc", &tqdc);
			}
			outtree->Branch("lab", &theta);
			outtree->Branch("com", &angleCOM);
			outtree->Branch("loc", &location);
		}

		TH2D *hE = NULL;
		TH2D *h2d = NULL;
		TH2D *hEcom = NULL;
		TH2D *h2dcom = NULL;
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

			int nBinsY = 550;
			double yLow = -10;
			double yHigh = 100;

			hE = new TH2D("hE", "Lab Angle vs. Energy", nBins, xbins, 500, 0, 10);
			hE->GetXaxis()->SetTitle("Lab Angle (deg)");
			hE->GetYaxis()->SetTitle("Neutron Energy (MeV)");
			hE->GetZaxis()->SetTitle("Counts per 20 keV");

			hEcom = new TH2D("hEcom", "Lab Angle vs. Energy", nBins, startAngle, stopAngle, 500, 0, 10);
			hEcom->GetXaxis()->SetTitle("CoM Angle (deg)");
			hEcom->GetYaxis()->SetTitle("Neutron Energy (MeV)");
			hEcom->GetZaxis()->SetTitle("Counts per 20 keV");

			h2d = new TH2D("h2d", "Lab Angle vs. ctof", nBins, xbins, nBinsY, yLow, yHigh);
			h2d->GetXaxis()->SetTitle("Lab Angle (deg)");
			h2d->GetYaxis()->SetTitle("Neutron ToF (ns)");
			h2d->GetZaxis()->SetTitle("Counts per 0.2 ns");

			h2dcom = new TH2D("h2dcom", "Lab Angle vs. ctof", nBins, startAngle, stopAngle, nBinsY, yLow, yHigh);
			h2dcom->GetXaxis()->SetTitle("CoM Angle (deg)");
			h2dcom->GetYaxis()->SetTitle("Neutron ToF (ns)");
			h2dcom->GetZaxis()->SetTitle("Counts per 0.2 ns");
		}

		int file_counter = 1;
		while(openInputFile()){
			std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

			if(!loadInputTree()){
				std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
				continue;
			}

			TBranch *branch = NULL;
			VandleStructure *ptr = NULL;
			std::vector<double> hitTheta;
			std::vector<int> detLocation;
	
			if(!mcarlo){
				intree->SetBranchAddress("vandle", &ptr, &branch);
			}
			else{
				intree->SetMakeClass(1);
				intree->SetBranchAddress("hitTheta", &hitTheta, &branch);
				intree->SetBranchAddress("location", &detLocation);
			}
	
			if(!branch){
				std::cout << " Error: Failed to load branch \"vandle\" from input TTree.\n";
				return 8;
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
			
						// Check the tqdc threshold (if available).
						if(threshold > 0 && ptr->tqdc.at(j) < threshold) continue;
	
						// Check against the input tcutg (if available).
						if(useTCutG && !tcutg->IsInside(ptr->ctof.at(j), ptr->tqdc.at(j))) continue;
	
						rxn.SetLabAngle(ptr->theta.at(j));
						if(treeMode){
							ctof = ptr->ctof.at(j);
							energy = ptr->energy.at(j);
							tqdc = ptr->tqdc.at(j);
							theta = ptr->theta.at(j);
							location = ptr->loc.at(j);
			
							angleCOM = rxn.GetEjectile()->comAngle[0];
				
							outtree->Fill();
						}
						hE->Fill(ptr->theta.at(j), ptr->energy.at(j));
						h2d->Fill(ptr->theta.at(j), ptr->ctof.at(j));
						hEcom->Fill(rxn.GetEjectile()->comAngle[0], ptr->energy.at(j));
						h2dcom->Fill(rxn.GetEjectile()->comAngle[0], ptr->ctof.at(j));
					}
				}
				else{
					for(unsigned int j = 0; j < hitTheta.size(); j++){
						theta = hitTheta.at(j);
						rxn.SetLabAngle(theta);
						angleCOM = rxn.GetEjectile()->comAngle[0];
						location = detLocation.at(j);
					
						outtree->Fill();
					}
				}
			}
	
			pbar.finalize();
		
			std::cout << "  Rejected " << badCount << " events.\n";
		}
		
		outfile->cd();
		if(treeMode || mcarlo)
			outtree->Write();
		if(!mcarlo){
			hE->Write();
			h2d->Write();
			hEcom->Write();
			h2dcom->Write();
		}

		if(cut) delete cut;

		if(treeMode || mcarlo) std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
		if(!mcarlo) std::cout << "\n\n Done! Wrote " << h2d->GetEntries() << " entries to histogram.\n";
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
