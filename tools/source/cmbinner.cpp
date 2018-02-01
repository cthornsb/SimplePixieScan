#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TCutG.h"
#include "TNamed.h"

#include "cmcalc.hpp"
#include "simpleTool.hpp"
#include "CalibFile.hpp"
#include "Structures.h"

// E_ in MeV, return TQDC in keVee.
double energy2tqdc(const double &E_){
	const double a=0.95, b=8, c=0.1, d=0.9;
	return 1000*(a*E_-b*(1-std::exp(-c*std::pow(E_, d))));
}

// E_ in MeV and d in m, return TOF in ns.
double energy2tof(const double &E_, const double &d){
	const double coeff = 72.2982541205; // 
	return coeff*d/std::sqrt(E_);
}

double getBinWidth(const double &E_){
	const double p0 = 0.0186821;
	const double p1 = 0.0516979;
	const double p2 = 0.00815183;
	return (p0+p1*E_+p2*E_*E_);
}

double calcWidth(const double &E, const double &l){
	const double dl = 0.03;//0.05; // m
	const double dt = 1;//2; // ns
	const double Mn = 10454.0750977429; // MeV
	return (2*E/l)*std::sqrt(dl*dl+(2*E/Mn)*dt*dt);
}

double getBinWidth(const double &Ehigh, const double &l, const double &tolerance=1E-8){
	double x1, y1, x2, y2, El, Er, Ecenter;
	double Ewidth = calcWidth(Ehigh, l);

	x1 = Ehigh-Ewidth/2;
	y1 = x1 + calcWidth(x1, l)/2;

	double deltal, deltar;
	
	while(true){
		x2 = x1 - Ewidth/4;
		y2 = x2 + calcWidth(x2, l)/2;
		
		El = (Ehigh-y1)*(x2-x1)/(y2-y1) + x1;
		deltal = El + calcWidth(El, l)/2;

		x2 = x1 + Ewidth/4;
		y2 = x2 + calcWidth(x2, l)/2;

		Er = (Ehigh-y1)*(x2-x1)/(y2-y1) + x1;

		deltar = Er + calcWidth(Er, l)/2;

		if(deltal < deltar){
			if(std::fabs(Ehigh-deltal) <= tolerance){
				Ecenter = El;
				break;
			}
			x1 = x1 - Ewidth/4;
		}
		else{
			if(std::fabs(Ehigh-deltar) <= tolerance){
				Ecenter = Er;
				break;
			}		
			x1 = x1 + Ewidth/4;
		}

		y1 = x1 + calcWidth(x1, l)/2;
		Ewidth = Ewidth/2;
	}
	
	return calcWidth(Ecenter, l);
}

size_t binTOF(std::vector<double> &tofBinsLow, std::vector<double> &energyBinsLow, const double &r=0.5, const double &lowLimit=0.1, const double &highLimit=8.0, const double &mult_=1){
	tofBinsLow.clear();
	energyBinsLow.clear();

	double low = highLimit; // MeV
	double width;

	while(low > lowLimit){
		width = mult_*getBinWidth(low, r);
		tofBinsLow.push_back(energy2tof(low, r));
		energyBinsLow.push_back(low);
		low = low - width;
	}

	/*const double eBins[45] = {0.0000, 0.0150, 0.0350, 0.0550, 0.0750, 
	                          0.0950, 0.1150, 0.1350, 0.1650, 0.1950, 
        	                  0.2250, 0.2550, 0.3050, 0.3550, 0.4050, 
                	          0.4550, 0.5050, 0.5550, 0.6050, 0.6550, 
                        	  0.7050, 0.7550, 0.8050, 0.8550, 0.9050, 
	                          0.9550, 1.0500, 1.1500, 1.2500, 1.3500, 
        	                  1.4500, 1.5500, 1.6500, 1.7500, 1.8500, 
                	          1.9500, 2.1500, 2.3500, 2.5500, 2.7500, 
                        	  2.9500, 3.2500, 3.5500, 3.8500, 4.1500};

	for(size_t i = 45; i > 1; i--){
		tofBinsLow.push_back(energy2tof(eBins[i-1], r));
		energyBinsLow.push_back(eBins[i-1]);
	}*/

	// Reverse the energy bin vector so that the bins are in increasing order.
	std::reverse(energyBinsLow.begin(), energyBinsLow.end());	

	return tofBinsLow.size();
}

class branchHolder{
  public:
	branchHolder() : isGood(true) { }

	bool AddBranch(TTree *tree_, const std::string &name_, void *ptr_, bool isRequired_=true);

	bool IsGood(){ return isGood; }

  private:
	std::vector<std::string> names;
	std::vector<TBranch*> branches;
	std::vector<void*> ptrs;
	bool isGood;
};

bool branchHolder::AddBranch(TTree *tree_, const std::string &name_, void *ptr_, bool isRequired_/*=true*/){
	ptrs.push_back(ptr_);
	names.push_back(name_);
	TBranch *branch = NULL;
	int retval = tree_->SetBranchAddress(name_.c_str(), ptr_, &branch);
	branches.push_back(branch);
	if(retval < 0){
		if(isRequired_){
			std::cout << " Error: Failed to load branch \"" << name_ << "\" from input TTree (code=" << retval << ").\n";
			isGood = false;
		}
		return false;
	}
	return true;
}

class threshCal : public CalType {
  public:
	double thresh;

	threshCal() : CalType(0), thresh(1E6) { }

	virtual std::string Print(bool fancy=true);

	virtual void ReadPars(const std::vector<std::string> &pars_);
};

std::string threshCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", thresh=" << thresh;
	else output << id << "\t" << thresh;
	return output.str();
}

void threshCal::ReadPars(const std::vector<std::string> &pars_){
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else if(index == 1) thresh = strtod(iter->c_str(), NULL);
		index++;
	}
}

class simpleComCalculator : public simpleTool {
  private:
	double startAngle;
	double stopAngle;
	double binWidth;
	double threshold;
	double radius;
	double userEnergy;
	double maxAxialPosition;
	double timeOffset;
	double widthMultiplier;
	int nBins;

	bool mcarlo;
	bool vandmc;
	bool printMode;
	bool defaultMode;
	bool treeMode;
	bool reactionMode;
	bool thresholdFile;
	bool tqdcGateFile;
	
	interpolator *tqdcGate;

	std::string configFilename;
	std::string thresholdFilename;
	std::string binWidthFilename;
	std::string tqdcGateFilename;

	int restrictLocation;

	reaction rxn;

	threshCal dummy;

	std::vector<threshCal> detThresh;

	bool setupReaction();

	void setAngles();

	threshCal *getThreshCal(const unsigned int &id_);

  public:
	simpleComCalculator() : simpleTool(), startAngle(0), stopAngle(180), binWidth(1), 
	                        threshold(-1), radius(0.5), userEnergy(-1), maxAxialPosition(-1), 
	                        timeOffset(0), widthMultiplier(1), nBins(180), 
	                        mcarlo(false), vandmc(false), printMode(false), defaultMode(false), 
	                        treeMode(false), reactionMode(true), thresholdFile(false), tqdcGateFile(false),
	                        tqdcGate(NULL), configFilename(""), thresholdFilename(""), binWidthFilename(""),
	                        tqdcGateFilename(""), restrictLocation(-1) { }

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
		if(reactionMode) std::cout << " Enter starting CoM angle: ";
		else             std::cout << " Enter starting lab angle: ";
		std::cin >> startAngle;
		if(startAngle < 0){
			std::cout << "  Error: Illegal starting angle (" << startAngle << "). Value may not be negative.\n";
			continue;
		}
		break;
	}

	// Set stopping angle.
	while(true){
		if(reactionMode) std::cout << " Enter stopping CoM angle: ";
		else             std::cout << " Enter stopping lab angle: ";
		std::cin >> stopAngle;
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

threshCal *simpleComCalculator::getThreshCal(const unsigned int &id_){
	if(id_ >= detThresh.size()){ return NULL; }
	return &detThresh.at(id_);
}

simpleComCalculator::~simpleComCalculator(){
	if(tqdcGate) delete tqdcGate;
}

void simpleComCalculator::addOptions(){
	addOption(optionExt("mcarlo", no_argument, NULL, 'm', "", "Read from a VANDMC monte carlo file."), userOpts, optstr);
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read reaction information from an input file."), userOpts, optstr);
	addOption(optionExt("tree", no_argument, NULL, 't', "", "Fill a TTree with reaction data."), userOpts, optstr);
	addOption(optionExt("print", no_argument, NULL, 'p', "", "Instead of scanning a file, print out kinematics information."), userOpts, optstr);
	addOption(optionExt("default", no_argument, NULL, 'd', "", "When using \"--print\" or \"--tree\" mode, use default settings of theta=[0,180] w/ 1 degree steps."), userOpts, optstr);
	addOption(optionExt("threshold", required_argument, NULL, 'T', "<threshold>", "Use a software threshold on the trqce QDC (not used by default)."), userOpts, optstr);
	addOption(optionExt("radius", required_argument, NULL, 0x0, "<radius>", "Set the detector radius (default=0.5 m)."), userOpts, optstr);
	addOption(optionExt("energy", required_argument, NULL, 'E', "<energy>", "Specify the beam energy in MeV."), userOpts, optstr);
	addOption(optionExt("max-yaxis", required_argument, NULL, 0x0, "<radius>", "Specify the maximum detector axial position in m (not used by default)."), userOpts, optstr);
	addOption(optionExt("time-offset", required_argument, NULL, 0x0, "<offset>", "Specify the TOF offset in ns."), userOpts, optstr);
	addOption(optionExt("no-reaction", no_argument, NULL, 0x0, "", "Do not use reaction kinematics (no CM calculations)."), userOpts, optstr);
	addOption(optionExt("multiplier", required_argument, NULL, 0x0, "<factor>", "Specify bin width multiplier (1 by default)."), userOpts, optstr);
	addOption(optionExt("vandmc", no_argument, NULL, 0x0, "", "Read from a VANDMC output file."), userOpts, optstr);
	addOption(optionExt("xbins", required_argument, NULL, 0x0, "<filename>", "Read lab angle bin widths from a file."), userOpts, optstr);
	addOption(optionExt("location", required_argument, NULL, 0x0, "<location>", "Only process events from a particular pixie ID."), userOpts, optstr);
	addOption(optionExt("tqdc-gate", required_argument, NULL, 0x0, "<filename>", "Use a TQDC gate on the input data."), userOpts, optstr);
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
	if(userOpts.at(5).active){
		std::string thresholdString = userOpts.at(5).argument;
		bool isFullyNumerical = true;
		for(size_t i = 0; i < thresholdString.size(); i++){
			if((thresholdString[i] < 0x30 || thresholdString[i] > 0x39) && thresholdString[i] != 0x2E){
				isFullyNumerical = false;
				break;
			}
		}
		if(isFullyNumerical)
			threshold = strtod(userOpts.at(5).argument.c_str(), 0);
		else{
			thresholdFilename = thresholdString;
			thresholdFile = true;
		}
	}
	if(userOpts.at(6).active)
		radius = strtod(userOpts.at(6).argument.c_str(), 0);
	if(userOpts.at(7).active)
		userEnergy = strtod(userOpts.at(7).argument.c_str(), 0);
	if(userOpts.at(8).active)
		maxAxialPosition = strtod(userOpts.at(8).argument.c_str(), 0);
	if(userOpts.at(9).active)
		timeOffset = strtod(userOpts.at(9).argument.c_str(), 0);
	if(userOpts.at(10).active)
		reactionMode = false;
	if(userOpts.at(11).active)
		widthMultiplier = strtod(userOpts.at(11).argument.c_str(), 0);
	if(userOpts.at(12).active)
		vandmc = true;
	if(userOpts.at(13).active)
		binWidthFilename = userOpts.at(13).argument;
	if(userOpts.at(14).active)
		restrictLocation = strtol(userOpts.at(14).argument.c_str(), NULL, 0);
	if(userOpts.at(15).active){
		tqdcGateFilename = userOpts.at(15).argument;
		tqdcGateFile = true;
	}

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

		if(!thresholdFilename.empty() && !LoadCalibFile<threshCal>(thresholdFilename.c_str(), detThresh)){
			std::cout << " Error: Failed to load threshold calibration file \"" << thresholdFilename << "\"!\n";
			return 3;
		}

		// Load a curve to use as a TQDC gate.
		if(tqdcGateFile){
			tqdcGate = new interpolator(tqdcGateFilename.c_str());
			if(tqdcGate->empty()){
				std::cout << " Error: Failed to load entries from TQDC gate file \"" << tqdcGateFilename << "\".\n";
				return 4;
			}
		}

		if(reactionMode && !setupReaction())
			return 5;

		if(!openOutputFile()){
			std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
			return 6;
		}

		// Write the threshold to the output file.
		if(threshold > 0){
			std::stringstream stream;
			stream << threshold;
			TNamed named("threshold", stream.str().c_str());
			named.Write();
			named.SetName("radius");
			stream.str("");
			stream << radius;
			named.SetTitle(stream.str().c_str());
			named.Write();
		}

		double ctof, energy, ctqdc, tqdc, theta, angleCOM, yaxis;
		unsigned short location;

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

		TH2F *hElab = NULL;
		TH2F *h2dlab = NULL;
		TH2F *hEcom = NULL;
		TH2F *h2dcom = NULL;
		TH2F *hEloc = NULL;
		TH2F *h2dloc = NULL;
		TH2F *hEtqdc = NULL;
		TH2F *h2dtqdc = NULL;
		if(!mcarlo){
			std::cout << std::endl;

			bool reverseLabAngles = false;
			bool reverseCOMAngles = false;
			std::vector<double> xbinsLowLab;
			std::vector<double> xbinsLow;
			if(!binWidthFilename.empty()){
				std::ifstream binFile(binWidthFilename.c_str());
				if(!binFile.good()){
					std::cout << " Error: Failed to load bin width file \"" << binWidthFilename << "\".\n";
					return 6;
				}

				nBins = 0;
				double readValueLab, readValueCOM;
				while(true){
					binFile >> readValueLab >> readValueCOM;
					if(binFile.eof()) break;
					xbinsLowLab.push_back(readValueLab);
					xbinsLow.push_back(readValueCOM);
					nBins++;
				}

				binFile.close();
	
				// Check for at least one angular bin.
				nBins = nBins - 1;
				if(nBins < 1){
					std::cout << " Error: Invalid number of angle bins (" << nBins << ").\n";
					return 7;
				}

				// Check that the angles are in increasing order.
				double prevLab = xbinsLowLab.front();
				if(reactionMode){
					double prevCOM = xbinsLow.front();
					for(int i = 1; i <= nBins; i++){
						if(prevLab >= xbinsLowLab.at(i)) reverseLabAngles = true;
						if(prevCOM >= xbinsLow.at(i)) reverseCOMAngles = true;
						prevLab = xbinsLowLab.at(i);
						prevCOM = xbinsLow.at(i);
					}
				}
				else{
					for(int i = 1; i <= nBins; i++){
						if(prevLab >= xbinsLowLab.at(i)) reverseLabAngles = true;
						prevLab = xbinsLowLab.at(i);
					}
				}
			}
			else{
				if(!defaultMode) setAngles();
				xbinsLow.resize(nBins);

				if(reactionMode){ // Fixed width CM angle bins.
					particle *ejectile = rxn.GetEjectile();

					double comAngle = startAngle;
					for(int i = 0; i <= nBins; i++){
						if(binWidthFilename.empty())
							comAngle = startAngle + i*binWidth;
						else
							comAngle = xbinsLow[i];

						// Handle inverse kinematics case.
						if(ejectile->inverseKin) comAngle = 180 - comAngle;

						rxn.SetComAngle(comAngle);
						xbinsLow.push_back(comAngle);
						xbinsLowLab.push_back(ejectile->labAngle[0]);
					}

					// Reverse the bins in inverse kinematics so that they are in increasing order.
					if(ejectile->inverseKin) reverseLabAngles = true;
				}
				else{ // No CM calculation.
					xbinsLowLab.resize(nBins+1);
					for(int i = 0; i <= nBins; i++){
						if(binWidthFilename.empty()) // Fixed width lab angle bins.
							xbinsLowLab[i] = startAngle + i*(stopAngle-startAngle)/nBins;
						else
							xbinsLowLab[i] = xbinsLow.at(i);
					}
				}
			}

			// Print the angle bins to the screen.
			if(reactionMode){
				std::cout << "bin\tlowCom\tlowLab\n";
				for(int i = 0; i <= nBins; i++)	std::cout << i << "\t" << xbinsLow.at(i) << "\t" << xbinsLowLab[i] << "\n";
			}
			else{
				std::cout << "bin\tlowLab\n";
				for(int i = 0; i <= nBins; i++)	std::cout << i << "\t" << xbinsLowLab.at(i) << "\n";	
			}

			// Reverse the lab angle bins so that they are in increasing order.
			if(reverseLabAngles) std::reverse(xbinsLowLab.begin(), xbinsLowLab.end());

			// Reverse the COM angle bins so that they are in increasing order.
			if(reverseCOMAngles) std::reverse(xbinsLow.begin(), xbinsLow.end());

			std::vector<double> tofBins, energyBins;
			binTOF(tofBins, energyBins, radius, 0.1, 8, widthMultiplier);

			// Create lab histograms.
			hElab = new TH2F("hElab", "Energy vs. Lab Angle", nBins, xbinsLowLab.data(), energyBins.size()-1, energyBins.data());
			hElab->GetXaxis()->SetTitle("Lab Angle (deg)");
			hElab->GetYaxis()->SetTitle("Neutron Energy (MeV)");

			h2dlab = new TH2F("h2dlab", "Corrected TOF vs. Lab Angle", nBins, xbinsLowLab.data(), tofBins.size()-1, tofBins.data());
			h2dlab->GetXaxis()->SetTitle("Lab Angle (deg)");
			h2dlab->GetYaxis()->SetTitle("Neutron TOF (ns)");

			// Create location histograms.
			hEloc = new TH2F("hEloc", "Energy vs. Location", 100, 0, 100, energyBins.size()-1, energyBins.data());
			hEloc->GetXaxis()->SetTitle("Detector Location");
			hEloc->GetYaxis()->SetTitle("Neutron Energy (MeV)");

			h2dloc = new TH2F("h2dloc", "Corrected TOF vs. Location", 100, 0, 100, tofBins.size()-1, tofBins.data());
			h2dloc->GetXaxis()->SetTitle("Detector Location");
			h2dloc->GetYaxis()->SetTitle("Neutron TOF (ns)");

			if(reactionMode){ // Create CM histograms.
				h2dcom = new TH2F("h2dcom", "Corrected TOF vs. CM Angle", nBins, xbinsLow.data(), tofBins.size()-1, tofBins.data());
				hEcom = new TH2F("hEcom", "Energy vs. CM Angle", nBins, xbinsLow.data(), energyBins.size()-1, energyBins.data());

				h2dcom->GetXaxis()->SetTitle("COM Angle (deg)");
				h2dcom->GetYaxis()->SetTitle("Neutron TOF (ns)");
				hEcom->GetXaxis()->SetTitle("COM Angle (deg)");
				hEcom->GetYaxis()->SetTitle("Neutron Energy (MeV)");
			}

			// Generate TQDC bins.
			std::vector<double> tqdcBins;
			tqdcBins.reserve(energyBins.size());
			for(size_t i = 0; i < energyBins.size(); i++){
				tqdcBins.push_back(energy2tqdc(energyBins.at(i)));
			}

			// Create TQDC histograms.
			hEtqdc = new TH2F("hEtqdc", "Trace QDC vs. Energy", energyBins.size()-1, energyBins.data(), tqdcBins.size()-1, tqdcBins.data());
			hEtqdc->GetXaxis()->SetTitle("Neutron Energy (MeV)");
			hEtqdc->GetYaxis()->SetTitle("Trace QDC (keVee)");

			h2dtqdc = new TH2F("h2dtqdc", "Trace QDC vs. Corrected TOF", tofBins.size()-1, tofBins.data(), tqdcBins.size()-1, tqdcBins.data());
			h2dtqdc->GetXaxis()->SetTitle("Neutron TOF (ns)");
			h2dtqdc->GetYaxis()->SetTitle("Trace QDC (keVee)");
		}

		int file_counter = 1;
		while(openInputFile()){
			std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

			if(!loadInputTree()){
				std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
				continue;
			}

			branchHolder branches;
			std::vector<double> hitTheta;
			std::vector<int> detLocation;
			
			// VANDMC data types
			std::vector<double> tof_v, energy_v, qdc_v, hitY_v;
	
			bool hasCorTqdcBranch = false;
			if(!mcarlo){
				if(!vandmc){
					// Required branches.
					branches.AddBranch(intree, "ctof", &ctof);
					branches.AddBranch(intree, "energy", &energy);
					branches.AddBranch(intree, "tqdc", &tqdc);
					branches.AddBranch(intree, "theta", &theta);
					branches.AddBranch(intree, "loc", &location);
					branches.AddBranch(intree, "y", &yaxis);

					// Optional branch.
					hasCorTqdcBranch = branches.AddBranch(intree, "ctqdc", &ctqdc, false);
				}
				else{
					intree->SetMakeClass(1);
					branches.AddBranch(intree, "tof", &tof_v);
					branches.AddBranch(intree, "energy", &energy_v);
					branches.AddBranch(intree, "qdc", &qdc_v);
					branches.AddBranch(intree, "hitTheta", &hitTheta);
					branches.AddBranch(intree, "loc", &detLocation);
					branches.AddBranch(intree, "hitY", &hitY_v);
				}
			}
			else{
				intree->SetMakeClass(1);
				branches.AddBranch(intree, "hitTheta", &hitTheta);
				branches.AddBranch(intree, "location", &detLocation);
			}

			// Check for all required branches.
			if(!branches.IsGood()) continue;
	
			unsigned int badCount = 0;
			while(getNextEntry()){
				if(!mcarlo){
					if(vandmc){
						if(tof_v.empty()) continue;
						ctof = tof_v.front();
						energy = energy_v.front();
						tqdc = qdc_v.front();
						theta = hitTheta.front();
						location = detLocation.front();
						yaxis = hitY_v.front();
					}
				
					// Check the location of this event.
					if(restrictLocation >= 0 && location != restrictLocation) continue;
	
					if(maxAxialPosition > 0 && std::fabs(yaxis) > maxAxialPosition){
						badCount++;
						continue;
					}

					// Check the TQDC gate (if available).
					if(hasCorTqdcBranch && tqdcGateFile){
						double tqdcLimit;
						if(!tqdcGate->interpolate(energy, tqdcLimit)) continue;
						tqdcLimit *= 1.1; // Add 10% to the cutoff.
						if(ctqdc > tqdcLimit) continue;
					}

					// Check the TQDC threshold (if available).
					if(threshold > 0 && tqdc < threshold) continue;
					else if(thresholdFile){
						threshCal *det = getThreshCal(location);
						if(!det || tqdc < det->thresh) continue;
					}

					rxn.SetLabAngle(theta);
					if(treeMode){
						angleCOM = rxn.GetEjectile()->comAngle[0];
						outtree->Fill();
					}
					hElab->Fill(theta, energy);
					h2dlab->Fill(theta, ctof);
					hEloc->Fill(location, energy);
					h2dloc->Fill(location, ctof);
					if(reactionMode){
						hEcom->Fill(rxn.GetEjectile()->comAngle[0], energy);
						h2dcom->Fill(rxn.GetEjectile()->comAngle[0], ctof);
					}
					if(hasCorTqdcBranch){
						hEtqdc->Fill(energy, ctqdc);
						h2dtqdc->Fill(ctof, ctqdc);
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
			std::cout << "  Rejected " << badCount << " events.\n";
		}
		
		outfile->cd();
		if(treeMode || mcarlo)
			outtree->Write();
		if(!mcarlo){
			hElab->Write();
			h2dlab->Write();
			hEloc->Write();
			h2dloc->Write();
			if(reactionMode){
				hEcom->Write();
				h2dcom->Write();
			}
			hEtqdc->Write();
			h2dtqdc->Write();
		}

		if(treeMode || mcarlo) std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
		if(!mcarlo) std::cout << "\n\n Done! Wrote " << h2dlab->GetEntries() << " entries to histogram.\n";
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
