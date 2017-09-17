#include <iostream>
#include <vector>
#include <time.h>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "CTerminal.h"

#include "simpleTool.hpp"
#include "CalibFile.hpp"
#include "Structures.h"

const double pi = 3.1415926536;

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

	GenericBarStructure *ptr;

	double x, y, z, r, theta;
	double ctof, tqdc, energy;
	unsigned short location;

	double tdiff_L, tdiff_R;
	float tqdc_L, tqdc_R;

	std::vector<barCal> bars;

	barCal *getBarCal(const unsigned int &id_);

	bool getNextEvent();

	void handleEvents();

  public:
	barHandler() : simpleTool(), setupDir("./setup/"), index(0), calib(), dummy(), ptr(NULL) { }

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
	if(index >= ptr->mult) return false;

	tdiff_L = ptr->ltdiff.at(index);
	tdiff_R = ptr->rtdiff.at(index);
	tqdc_L = ptr->ltqdc.at(index);
	tqdc_R = ptr->rtqdc.at(index);
	location = ptr->loc.at(index);

	index++;

	return true;
}

void barHandler::handleEvents(){
	index = 0;
	double ctdiff, cylTheta, dW, alpha, rprime;
	while(getNextEvent()){
		barCal *bar = getBarCal(location);
		TimeCal *time = calib.GetTimeCal(location);
		PositionCal *pos = calib.GetPositionCal(location);

		if(!bar || !time || !pos) continue;

		// Angle of center of bar in cylindrical coordinates.
		cylTheta = pos->theta;

		// Take the width of the bar into consideration.
		dW = frand(-bar->width/200, bar->width/200);
		rprime = std::sqrt(pos->r0*pos->r0 + dW*dW);
		alpha = std::atan2(dW, pos->r0);
		addAngles(cylTheta, alpha);

		// Compute the corrected time difference
		ctdiff = tdiff_R - tdiff_L - bar->t0;

		// Calculate the position of the event.
		x = rprime*std::sin(cylTheta); // m
		z = rprime*std::cos(cylTheta); // m
		y = bar->cbar*ctdiff/200; // m
		//theta = std::acos(std::cos(pos->theta)/std::sqrt(1.0+ypos*ypos/(r0*r0)));
		//phi = std::atan2(ypos, r0*std::sin(theta0));

		r = std::sqrt(x*x + y*y + z*z);
		theta = std::acos(z/r)*180/pi;
		//phi = std::acos(y/std::sqrt(x*x + y*y); }
		//if(x < 0) phi = 2*pi - phi; 

		// Calculate the corrected TOF.
		//ctof = (pos->r0/std::sqrt(ctdiff*ctdiff + pos->r0*pos->r0))*(0.5*(tdiff_R + tdiff_L)-detectorLength/(2*bar->cbar));
		ctof = (pos->r0/r)*((tdiff_R + tdiff_L)/2 - time->t0) + 100*pos->r0/bar->cbar;

		// Calculate the TQDC and neutron energy.
		tqdc = std::sqrt(tqdc_R*tqdc_L);
		energy = tof2energy(ctof, pos->r0);

		// Fill the tree with the event.
		outtree->Fill();
	}
}

barHandler::~barHandler(){
}

void barHandler::addOptions(){
	addOption(optionExt("config", required_argument, NULL, 'c', "<fname>", "Read bar speed-of-light from an input cal file."), userOpts, optstr);
}

bool barHandler::processArgs(){
	if(userOpts.at(0).active){
		setupDir = userOpts.at(0).argument;
		if(setupDir[setupDir.size()-1] != '/') setupDir += '/';
	}

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

	if(!calib.LoadPositionCal((setupDir+"position.cal").c_str())){
		std::cout << " Error: Failed to load position calibration file \"" << setupDir << "position.cal\"!\n";
		return 2;
	}

	if(!calib.LoadTimeCal((setupDir+"time.cal").c_str())){
		std::cout << " Error: Failed to load time calibration file \"" << setupDir << "time.cal\"!\n";
		return 3;
	}

	if(!LoadCalibFile<barCal>((setupDir+"bars.cal").c_str(), bars)){
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
	outtree->Branch("r", &r);
	outtree->Branch("theta", &theta);
	outtree->Branch("x", &x);
	outtree->Branch("y", &y);
	outtree->Branch("z", &z);
	outtree->Branch("loc", &location);

	int file_counter = 1;
	while(openInputFile()){
		std::cout << "\n " << file_counter++ << ") Processing file " << input_filename << std::endl;

		if(!loadInputTree()){
			std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
			continue;
		}

		TBranch *branch = NULL;
		intree->SetBranchAddress("genericbar", &ptr, &branch);

		if(!branch){
			std::cout << " Error: Failed to load branch \"genericbar\" from input TTree.\n";
			return 7;
		}

		progressBar pbar;
		pbar.start(intree->GetEntries());

		for(unsigned int i = 0; i < intree->GetEntries(); i++){
			pbar.check(i);

			intree->GetEntry(i);
	
			handleEvents();
		}

		pbar.finalize();
	}
		
	outfile->cd();
	outtree->Write();

	std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";
			
	return 0;
}

int main(int argc, char *argv[]){
	barHandler obj;
	
	return obj.execute(argc, argv);
}
