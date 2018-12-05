#include <iostream>
#include <vector>
#include <time.h>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "CTerminal.h"

#include "simpleTool.hpp"
#include "CalibFile.hpp"
#include "Structures.hpp"

template <typename T>
void writeTNamed(const char *label_, const T &val_, const int &precision_=-1){
	std::stringstream stream; 
	if(precision_ < 0) 
		stream << val_;
	else{ // Set the precision of the output stream.
		stream.precision(precision_);
		stream << std::fixed << val_;
	}
	TNamed named(label_, stream.str().c_str());
	named.Write();
}

class barHandler : public simpleTool {
  private:
	std::string setupDir;

	unsigned short index;

	CalibFile calib;

	GenericBarStructure *gbptr;
	GenericStructure *gptr;
	LiquidBarStructure *lbptr;
	LiquidStructure *lptr;
	HagridStructure *hptr;

	int detectorType;
	bool singleEndedMode;
	bool liquidDetMode;
	bool noTimeMode;
	bool noEnergyMode;
	bool noPositionMode;
	bool useLightBalance;

	std::string countsString;
	unsigned long long totalCounts;
	double totalDataTime;

	double x, y, z, r, theta, phi;
	double tdiff, tof, ctof, tqdc, stqdc, lbal, ctqdc, energy;
	double centerE;
	unsigned short location;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;
	float stqdc_L, stqdc_R;	

	bool getNextEvent();

	void handleEvents();

  public:
	barHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), gbptr(NULL), gptr(NULL), lbptr(NULL), lptr(NULL), hptr(NULL), detectorType(0), singleEndedMode(false), liquidDetMode(false), noTimeMode(false), noEnergyMode(false), noPositionMode(false), useLightBalance(false), countsString(""), totalCounts(0), totalDataTime(0) { }

	~barHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

bool barHandler::getNextEvent(){
	if(detectorType == 0){ // GenericBar
		if(index >= gbptr->mult) return false;
		tdiff_L = gbptr->ltdiff.at(index);
		tdiff_R = gbptr->rtdiff.at(index);
		tqdc_L = gbptr->ltqdc.at(index);
		tqdc_R = gbptr->rtqdc.at(index);
		location = gbptr->loc.at(index);
	}
	else if(detectorType == 1){ // Generic
		if(index >= gptr->mult) return false;
		tdiff_L = gptr->tof.at(index);
		tqdc_L = gptr->tqdc.at(index);
		location = gptr->loc.at(index);
	}
	else if(detectorType == 2){ // LiquidBar
		if(index >= lbptr->mult) return false;
		tdiff_L = lbptr->ltdiff.at(index);
		tdiff_R = lbptr->rtdiff.at(index);
		tqdc_L = lbptr->lltqdc.at(index);
		tqdc_R = lbptr->rltqdc.at(index);
		stqdc_L = lbptr->lstqdc.at(index);
		stqdc_R = lbptr->rstqdc.at(index);
		location = lbptr->loc.at(index);
	}
	else if(detectorType == 3){ // Liquid
		if(index >= lptr->mult) return false;
		tdiff_L = lptr->tof.at(index);
		tqdc_L = lptr->ltqdc.at(index);
		stqdc_L = lptr->stqdc.at(index);
		location = lptr->loc.at(index);
	}
	else{ // HAGRiD
		if(index >= hptr->mult) return false;
		tdiff_L = hptr->tof.at(index);
		tqdc_L = hptr->energy.at(index);
		location = hptr->loc.at(index);
	}

	index++;

	return true;
}

void barHandler::handleEvents(){
	index = 0;
	while(getNextEvent()){
		// Check for invalid TQDC.
		if(tqdc_L <= 0 || (!singleEndedMode && tqdc_R <= 0)) continue;

		BarCal *bar = NULL;
		if(!singleEndedMode && !(bar = calib.GetBarCal(location))) continue;

		TimeCal *time = NULL;
		if(!noTimeMode && !(time = calib.GetTimeCal(location))) continue;

		PositionCal *pos = NULL;
		if(!noPositionMode){
			if(!(pos = calib.GetPositionCal(location))) continue;
		}

		// Compute the corrected time difference
		if(!singleEndedMode){
			if(!useLightBalance){
				double ctdiff = tdiff_R - tdiff_L - bar->t0;
				y = bar->cbar*ctdiff/200; // m
			}
			else{
				// Calculate the light balance.
				lbal = (tqdc_L-tqdc_R)/(tqdc_L+tqdc_R) - bar->t0;

				// Use light balance to compute position in detector.
				y = lbal*((bar->length/100)/bar->beta);
			}
		}
		else y = 0; // m
		
		if(!noPositionMode){
			// Take the width and thickness of the bar into consideration.
			// Select a random point inside the bar.
			double xdetRan = frand(-bar->width/200, bar->width/200);
			double ydetRan = frand(-bar->width/200, bar->width/200);

			// Vector from the center to the interaction point.
			Vector3 p(xdetRan, ydetRan, y);
	
			// Rotate to the frame of the bar.
			pos->Transform(p);
	
			// Calculate the vector from the origin of the lab frame.
			Vector3 r0 = (*pos->GetPosition()) + p;
			x = r0.axis[0]; // m
			y = r0.axis[1]; // m
			z = r0.axis[2]; // m		
			
			// Convert the event vector to spherical.
			Cart2Sphere(r0, r, theta, phi);	
		}
		else{
			r = 0;
			theta = 0;
			phi = 0;
			x = 0;
			z = 0;
		}

		if(!singleEndedMode){
			// Calculate the corrected TOF.
			tdiff = (tdiff_R - tdiff_L);
			tof = (tdiff_R + tdiff_L)/2;
			if(!noTimeMode){ // Correct timing offset.
				tof = tof - time->t0;
			}
			if(!noPositionMode){
				ctof = (pos->r0/r)*tof;
				tof += 100*pos->r0/cvac;
				ctof += 100*pos->r0/cvac;
			}
			else
				ctof = tof;

			// Calculate the TQDC.
			tqdc = std::sqrt(tqdc_R*tqdc_L);
		}
		else{
			tof = tdiff_L;
			if(!noTimeMode){ // Correct timing offset.
				tof = tof - time->t0;
			}		
			if(!noPositionMode){
				tof += 100*pos->r0/cvac;
				ctof = tof;
			}
			else
				ctof = tof;
		
			// Calculate the TQDC.
			tqdc = tqdc_L;
		}

		if(liquidDetMode){ // Calculate the short integral for PSD.
			if(!singleEndedMode)
				stqdc = std::sqrt(stqdc_R*stqdc_L);
			else
				stqdc = stqdc_L;
		}

		if(!noEnergyMode){// Calibrate the TQDC.
			if(!singleEndedMode){
				EnergyCal *ecalLeft = calib.GetEnergyCal(location);
				EnergyCal *ecalRight = calib.GetEnergyCal(location+1);
				if(ecalLeft && !ecalLeft->defaultVals){
					if(!ecalRight || ecalRight->defaultVals){ // Use pairwise calibration.
						ctqdc = ecalLeft->GetCalEnergy(std::sqrt(tqdc_R*tqdc_L));
					}
					else{ // Use individual channel calibration.
						tqdc_L = ecalLeft->GetCalEnergy(tqdc_L);
						tqdc_R = ecalRight->GetCalEnergy(tqdc_R);
						ctqdc = std::sqrt(tqdc_R*tqdc_L);
					}
				}
			}
			else{
				EnergyCal *ecal = calib.GetEnergyCal(location);
				if(ecal) ctqdc = ecal->GetCalEnergy(tqdc);
			}
		}

		// Calculate the neutron energy.
		if(!noPositionMode) energy = tof2energy(tof, r);

		// Fill the tree with the event.
		outtree->Fill();
	}
}

barHandler::~barHandler(){
}

void barHandler::addOptions(){
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read bar speed-of-light from an input cal file."), userOpts, optstr);
	addOption(optionExt("single", no_argument, NULL, 0x0, "", "Single-ended detector mode."), userOpts, optstr);
	addOption(optionExt("debug", no_argument, NULL, 0x0, "", "Enable debug output."), userOpts, optstr);
	addOption(optionExt("liquid", no_argument, NULL, 0x0, "", "Liquid detector mode."), userOpts, optstr);
	addOption(optionExt("hagrid", no_argument, NULL, 0x0, "", "HAGRiD detector mode."), userOpts, optstr);
	addOption(optionExt("counts", required_argument, NULL, 0x0, "<path>", "Sum counts from the input root files (not used by default)."), userOpts, optstr);
	addOption(optionExt("light", no_argument, NULL, 0x0, "", "Do not use position calibration."), userOpts, optstr);	
	addOption(optionExt("no-energy", no_argument, NULL, 0x0, "", "Do not use energy calibration."), userOpts, optstr);
	addOption(optionExt("no-time", no_argument, NULL, 0x0, "", "Do not use time calibration."), userOpts, optstr);
	addOption(optionExt("no-position", no_argument, NULL, 0x0, "", "Do not use position calibration."), userOpts, optstr);
}

bool barHandler::processArgs(){
	bool single = false;
	bool liquid = false;
	bool hagrid = false;
	if(userOpts.at(0).active){
		setupDir = userOpts.at(0).argument;
		if(setupDir[setupDir.size()-1] != '/') setupDir += '/';
	}
	if(userOpts.at(1).active){ // Single-ended
		single = true;
	}
	if(userOpts.at(2).active){ // Debug output
		debug = true;
	}	
	if(userOpts.at(3).active){ // Liquid detector
		liquid = true;
	}
	if(userOpts.at(4).active){ // HAGRiD detector
		hagrid = true;
	}
	if(userOpts.at(5).active){
		countsString = userOpts.at(5).argument;
	}
	if(userOpts.at(6).active){
		useLightBalance = true;
	}
	if(userOpts.at(7).active){	
		noEnergyMode = true;
	}
	if(userOpts.at(8).active){
		noTimeMode = true;
	}
	if(userOpts.at(9).active){
		noPositionMode = true;
	}

	detectorType = 0;
	if(!single && !liquid && !hagrid) detectorType = 0;
	else if(single && !liquid && !hagrid) detectorType = 1;
	else if(!single && liquid && !hagrid) detectorType = 2;
	else if(single && liquid && !hagrid) detectorType = 3;
	else if(single && !liquid && hagrid) detectorType = 4;
	else{
		std::cout << " Error: Unrecognized detector type (SLH=" << single << liquid << hagrid << ")\n";
		return false;
	}

	singleEndedMode = single;
	liquidDetMode = liquid;

	//std::cout << " debug: detectorType=" << detectorType << " (SLH=" << single << liquid << hagrid << ")\n";

	return true;
}

int barHandler::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	// Seed the random number generator.
	srand(time(NULL));

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(!noPositionMode && !calib.LoadPositionCal((setupDir+"position.cal").c_str())) return 2;
	if(!noTimeMode && !calib.LoadTimeCal((setupDir+"time.cal").c_str())) return 3;
	if(!noEnergyMode && !calib.LoadEnergyCal((setupDir+"energy.cal").c_str())) return 4;
	if(!singleEndedMode && !calib.LoadBarCal((setupDir+"bars.cal").c_str())) return 5;

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 6;
	}
	
	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 7;
	}

	outtree = new TTree("data", "Barified data");

	outtree->Branch("ctof", &ctof);
	if(!noPositionMode)
		outtree->Branch("energy", &energy);
	outtree->Branch("tqdc", &tqdc);
	if(liquidDetMode)
		outtree->Branch("stqdc", &stqdc);
	if(!noEnergyMode)
		outtree->Branch("ctqdc", &ctqdc);
	if(!noPositionMode){
		outtree->Branch("r", &r);
		outtree->Branch("theta", &theta);
		outtree->Branch("phi", &phi);
		outtree->Branch("x", &x);
		outtree->Branch("y", &y);
		outtree->Branch("z", &z);
	}
	else if(!singleEndedMode)
		outtree->Branch("y", &y);
	if(debug){ // Diagnostic branches
		outtree->Branch("tof", &tof);
		if(useLightBalance)
			outtree->Branch("lbal", &lbal);
		outtree->Branch("cenE", &centerE);
		if(!singleEndedMode){
			outtree->Branch("tdiff", &tdiff);
		}
	}
	outtree->Branch("loc", &location);

	int file_counter = 1;
	while(openInputFile()){
		std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

		if(!loadInputTree()){ // Load the input tree.
			std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
			continue;
		}

		TBranch *branch = NULL;
		if(detectorType == 0) // GenericBar
			intree->SetBranchAddress("genericbar", &gbptr, &branch);
		else if(detectorType == 1) // Generic
			intree->SetBranchAddress("generic", &gptr, &branch);
		else if(detectorType == 2) // LiquidBar
			intree->SetBranchAddress("liquidbar", &lbptr, &branch);
		else if(detectorType == 3) // Liquid
			intree->SetBranchAddress("liquid", &lptr, &branch);
		else // HAGRiD
			intree->SetBranchAddress("hagrid", &hptr, &branch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"";
			if(detectorType == 0) // GenericBar
				std::cout << "genericbar";
			else if(detectorType == 1) // Generic
				std::cout << "generic";
			else if(detectorType == 2) // LiquidBar
				std::cout << "liquidbar";
			else if(detectorType == 3) // Liquid
				std::cout << "liquid";
			else // HAGRiD
				std::cout << "hagrid";
			std::cout << "\" from input TTree.\n";
			return 8;
		}

		TNamed *named;
		if(!countsString.empty()){ // Get the counts from the input file.
			infile->GetObject(("counts/"+countsString+"/Total").c_str(), named);
			if(named)
				totalCounts += strtoull(named->GetTitle(), NULL, 10);
			else 
				std::cout << " Warning: Failed to find object named \"counts/" << countsString << "/Total\" in input file!\n";
		}

		// Get the data time from the input file.
		infile->GetObject("head/file01/Data time", named);
		if(named){
			double fileDataTime = strtod(named->GetTitle(), NULL);
			if(fileDataTime > 0)
				totalDataTime += fileDataTime;
			else
				std::cout << " Warning: Encountered negative data time (" << fileDataTime << " s)\n";
		}
		else 
			std::cout << " Warning: Failed to find pixie data time in input file!\n";

		// Handle all events.
		while(getNextEntry())
			handleEvents();
	}

	// Write output tree to file.
	outfile->cd();
	outtree->Write();

	// Write calibration information to output file.
	calib.Write(outfile);

	// Write the total number of logic counts, if enabled.
	outfile->cd();
	if(totalCounts > 0) 
		writeTNamed(countsString.c_str(), totalCounts);

	// Write the total data time.
	writeTNamed("time", totalDataTime, 1);

	std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
			
	return 0;
}

int main(int argc, char *argv[]){
	barHandler obj;

	return obj.execute(argc, argv);
}
