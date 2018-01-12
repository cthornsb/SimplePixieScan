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
#include "Structures.h"

const double pi = 3.1415926536;
const double cvac = 29.9792458; // cm/ns

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

// Energy in MeV
double tof2energy(const double &tof_, const double &d_){
	const double Mn = 10454.0750977429; // MeV
	return (0.5*Mn*d_*d_/(tof_*tof_));
}

// Add dtheta_ to theta and wrap between 0 and 2pi
void addAngles(double &theta, const double &dtheta_){
	theta += dtheta_;
	if(theta < 0) theta += 2*pi;
	else if(theta > 2*pi) theta -= 2*pi;
}

// Return a random number between low and high
double frand(double low, double high){
	return low+(double(rand())/RAND_MAX)*(high-low);
}

void sphere2Cart(const double &r_, const double &theta_, const double &phi_, double &x, double &y, double &z){ 
	x = r_*std::sin(theta_)*std::cos(phi_);
	y = r_*std::sin(theta_)*std::sin(phi_);
	z = r_*std::cos(theta_);
} 

class barCal : public CalType {
  public:
	double t0;
	double beta;
	double cbar;
	double length;
	double width;

	barCal() : CalType(0), t0(0), beta(0), cbar(0), length(0), width(0) { }

	virtual std::string Print(bool fancy=true);

	virtual void ReadPars(const std::vector<std::string> &pars_);
};

std::string barCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", t0=" << t0 << ", beta=" << beta << ", cbar=" << cbar << ", length=" << length << ", width=" << width;
	else output << id << "\t" << t0 << "\t" << beta << "\t" << cbar << "\t" << length << "\t" << width;
	return output.str();
}

void barCal::ReadPars(const std::vector<std::string> &pars_){
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else if(index == 1) t0 = strtod(iter->c_str(), NULL);
		else if(index == 2) beta = strtod(iter->c_str(), NULL);
		else if(index == 3) cbar = strtod(iter->c_str(), NULL);
		else if(index == 4) length = strtod(iter->c_str(), NULL);
		else if(index == 5) width = strtod(iter->c_str(), NULL);
		index++;
	}
}

class barHandler : public simpleTool {
  private:
	std::string setupDir;

	unsigned short index;

	CalibFile calib;

	barCal dummy;

	GenericBarStructure *gbptr;
	LiquidBarStructure *lbptr;
	LiquidStructure *lptr;
	HagridStructure *hptr;

	int detectorType;
	bool singleEndedMode;
	bool liquidDetMode;
	bool noTimeMode;
	bool noEnergyMode;
	bool noPositionMode;

	std::string countsString;
	unsigned long long totalCounts;
	double totalDataTime;

	double x, y, z, r, theta;
	double ctof, tqdc, stqdc, ctqdc, energy;
	unsigned short location;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;
	float stqdc_L, stqdc_R;	

	std::vector<barCal> bars;

	barCal *getBarCal(const unsigned int &id_);

	bool getNextEvent();

	void handleEvents();

  public:
	barHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), dummy(), gbptr(NULL), lbptr(NULL), lptr(NULL), hptr(NULL), detectorType(0), singleEndedMode(false), liquidDetMode(false), noTimeMode(false), noEnergyMode(false), noPositionMode(false), countsString(""), totalCounts(0), totalDataTime(0) { }

	~barHandler();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

barCal *barHandler::getBarCal(const unsigned int &id_){
	if(id_ >= bars.size()){ return NULL; }
	return &bars.at(id_);
}

bool barHandler::getNextEvent(){
	if(detectorType == 0){ // GenericBar
		if(index >= gbptr->mult) return false;
		tdiff_L = gbptr->ltdiff.at(index);
		tdiff_R = gbptr->rtdiff.at(index);
		tqdc_L = gbptr->ltqdc.at(index);
		tqdc_R = gbptr->rtqdc.at(index);
		location = gbptr->loc.at(index);
	}
	else if(detectorType == 1){ // LiquidBar
		if(index >= lbptr->mult) return false;
		tdiff_L = lbptr->ltdiff.at(index);
		tdiff_R = lbptr->rtdiff.at(index);
		tqdc_L = lbptr->lltqdc.at(index);
		tqdc_R = lbptr->rltqdc.at(index);
		stqdc_L = lbptr->lstqdc.at(index);
		stqdc_R = lbptr->rstqdc.at(index);
		location = lbptr->loc.at(index);
	}
	else if(detectorType == 2){ // Liquid
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
	double ctdiff, cylTheta, dW, alpha, rprime;
	while(getNextEvent()){
		// Check for invalid TQDC.
		if(tqdc_L <= 0 || (!singleEndedMode && tqdc_R <= 0)) continue;

		barCal *bar = NULL;
		if(!singleEndedMode && !(bar = getBarCal(location))) continue;

		TimeCal *time = NULL;
		if(!noTimeMode && !(time = calib.GetTimeCal(location))) continue;
		
		PositionCal *pos = NULL;
		if(!noPositionMode){
			if(!(pos = calib.GetPositionCal(location))) continue;
	
			// Angle of center of bar in cylindrical coordinates.
			cylTheta = pos->theta;

			// Take the width of the bar into consideration.
			if(!singleEndedMode){
				dW = frand(-bar->width/200, bar->width/200);
				rprime = std::sqrt(pos->r0*pos->r0 + dW*dW);
				alpha = std::atan2(dW, pos->r0);
				addAngles(cylTheta, alpha);
			}
			else rprime = pos->r0;

			// Calculate the x and z position of the event.
			x = rprime*std::sin(cylTheta); // m
			z = rprime*std::cos(cylTheta); // m
		}
		else{
			x = 0;
			z = 0;
		}

		// Compute the corrected time difference
		if(!singleEndedMode){
			ctdiff = tdiff_R - tdiff_L - bar->t0;
			y = bar->cbar*ctdiff/200; // m

			// Check for invalid radius.
			if(y < -bar->length/30.0 || y > bar->length/30.0) continue;
		}
		else y = 0; // m

		// Calculate the spherical polar angle.
		if(!noPositionMode){
			r = std::sqrt(x*x + y*y + z*z);
			theta = std::acos(z/r)*180/pi;
			//phi = std::acos(y/std::sqrt(x*x + y*y); }
			//if(x < 0) phi = 2*pi - phi; 
		}
		else{
			r = 0;
			theta = 0;
		}

		if(!singleEndedMode){
			// Calculate the corrected TOF.
			if(!noPositionMode){
				if(!noTimeMode)
					ctof = (pos->r0/r)*((tdiff_R + tdiff_L)/2 - time->t0) + 100*pos->r0/cvac;
				else
					ctof = (pos->r0/r)*((tdiff_R + tdiff_L)/2) + 100*pos->r0/cvac;
			}
			else if(!noTimeMode)
				ctof = (tdiff_R + tdiff_L)/2 - time->t0;
			else
				ctof = (tdiff_R + tdiff_L)/2;

			// Calculate the TQDC.
			tqdc = std::sqrt(tqdc_R*tqdc_L);
		}
		else{
			if(!noPositionMode){
				if(!noTimeMode)
					ctof = tdiff_L - time->t0 + 100*pos->r0/cvac;
				else
					ctof = tdiff_L + 100*pos->r0/cvac;
			}
			else if(!noTimeMode)
				ctof = tdiff_L - time->t0;
			else
				ctof = tdiff_L;
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
		if(!noPositionMode) energy = tof2energy(ctof, pos->r0);

		// Fill the tree with the event.
		outtree->Fill();
	}
}

barHandler::~barHandler(){
}

void barHandler::addOptions(){
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read bar speed-of-light from an input cal file."), userOpts, optstr);
	addOption(optionExt("single", no_argument, NULL, 0x0, "", "Single-ended detector mode."), userOpts, optstr);
	addOption(optionExt("liquid", no_argument, NULL, 0x0, "", "Liquid detector mode."), userOpts, optstr);
	addOption(optionExt("hagrid", no_argument, NULL, 0x0, "", "HAGRiD detector mode."), userOpts, optstr);
	addOption(optionExt("counts", required_argument, NULL, 0x0, "<path>", "Sum counts from the input root files (not used by default)."), userOpts, optstr);
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
	if(userOpts.at(2).active){ // Liquid detector
		liquid = true;
	}
	if(userOpts.at(3).active){ // HAGRiD detector
		hagrid = true;
	}
	if(userOpts.at(4).active){
		countsString = userOpts.at(2).argument;
	}
	if(userOpts.at(5).active){
		noEnergyMode = true;
	}
	if(userOpts.at(6).active){
		noTimeMode = true;
	}
	if(userOpts.at(7).active){
		noPositionMode = true;
	}

	detectorType = 0;
	if(!single && !liquid && !hagrid) detectorType = 0;
	else if(!single && liquid && !hagrid) detectorType = 1;
	else if(single && liquid && !hagrid) detectorType = 2;
	else if(single && !liquid && hagrid) detectorType = 3;
	else{
		std::cout << " Error: Unrecognized detector type (SLH=" << single << liquid << hagrid << ")\n";
		return false;
	}

	singleEndedMode = (detectorType >= 2);
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
	if(!noEnergyMode){
		if(!calib.LoadEnergyCal((setupDir+"energy.cal").c_str()))
			noEnergyMode = true;
	}

	if(!singleEndedMode && !LoadCalibFile<barCal>((setupDir+"bars.cal").c_str(), bars)){
		std::cout << " Error: Failed to load bar calibration file \"" << setupDir << "bars.cal\"!\n";
		return 4;
	}

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 5;
	}
	
	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 6;
	}

	outtree = new TTree("data", "Barified data");

	outtree->Branch("ctof", &ctof);
	outtree->Branch("energy", &energy);
	outtree->Branch("tqdc", &tqdc);
	if(liquidDetMode)
		outtree->Branch("stqdc", &stqdc);
	if(!noEnergyMode)
		outtree->Branch("ctqdc", &ctqdc);
	outtree->Branch("r", &r);
	outtree->Branch("theta", &theta);
	outtree->Branch("x", &x);
	outtree->Branch("y", &y);
	outtree->Branch("z", &z);
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
		else if(detectorType == 1) // LiquidBar
			intree->SetBranchAddress("liquidbar", &lbptr, &branch);
		else if(detectorType == 2) // Liquid
			intree->SetBranchAddress("liquid", &lptr, &branch);
		else // HAGRiD
			intree->SetBranchAddress("hagrid", &hptr, &branch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"";
			if(detectorType == 0) // GenericBar
				std::cout << "genericbar";
			else if(detectorType == 1) // LiquidBar
				std::cout << "liquidbar";
			else if(detectorType == 2) // Liquid
				std::cout << "liquid";
			else // HAGRiD
				std::cout << "hagrid";
			std::cout << "\" from input TTree.\n";
			return 7;
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

	// Write bar calibration information to output file.
	outfile->mkdir("calib/bars");
	outfile->cd("calib/bars");

	// Write individual bar calibration entries.
	for(std::vector<barCal>::iterator iter = bars.begin(); iter != bars.end(); ++iter){
		if(iter->defaultVals) continue; // Skip entries with default values.
		TObjString str(iter->Print(false).c_str());
		str.Write();
	}

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
