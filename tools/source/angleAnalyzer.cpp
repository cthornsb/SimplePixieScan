#include <iostream>
#include <vector>
#include <time.h>
#include <cmath>

#include "TH1F.h"
#include "TCanvas.h"

#include "CTerminal.h"

#include "simpleTool.hpp"
#include "CalibFile.hpp"

class angleAnalyzer : public simpleTool {
  private:
	size_t maxEntry;	

	double detectorWidth;
	double detectorLength;

	CalibFile calib;

  public:
	angleAnalyzer() : simpleTool(), maxEntry(0), detectorWidth(3), detectorLength(60), calib() { }

	~angleAnalyzer();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

angleAnalyzer::~angleAnalyzer(){
}

void angleAnalyzer::addOptions(){
	addOption(optionExt("length", required_argument, NULL, 0, "<length>", "Specify the length of the detectors (in cm, default = 60)."), userOpts, optstr);
	addOption(optionExt("width", required_argument, NULL, 0, "<width>", "Specify the width of the detectors (in cm, default = 3)."), userOpts, optstr);
}

bool angleAnalyzer::processArgs(){
	if(userOpts.at(0).active)
		detectorLength = strtod(userOpts.at(0).argument.c_str(), NULL);
	if(userOpts.at(1).active)
		detectorWidth = strtod(userOpts.at(1).argument.c_str(), NULL);

	return true;
}

int angleAnalyzer::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(output_filename.empty()){
		output_filename = "angleAnalyzer.dat";
	}

	std::ofstream ofile(output_filename.c_str());
	if(!ofile.good()){
		std::cout << " Error: Failed to open output file \"" << output_filename << "\".\n";
		return 2;
	}

	if(!calib.LoadPositionCal(input_filename.c_str())) return 3;

	std::cout << "Using detector length of " << detectorLength << " cm.\n";
	std::cout << "Using detector width of " << detectorWidth << " cm.\n";

	double thetaCyl, theta, x, z, r;
	double rprime, alpha;

	ofile << "loc\tthetaLow\tthetaHigh\n";

	double dW[3] = {-detectorWidth/200, 0, detectorWidth/200};
	double dY[3] = {-detectorLength/200, 0, detectorLength/200};

	std::vector<double> minima;
	std::vector<double> maxima;

	double overallMin = 180;
	double overallMax = 0;

	maxEntry = calib.GetMaxPosition();
	for(size_t i = 0; i < maxEntry; i++){
		// Bar detectors must be even.
		if(i % 2 != 0) continue;

		PositionCal *pos = NULL;
		if(!(pos = calib.GetPositionCal(i)) || pos->defaultVals) continue;	

		double minAngle = 180;
		double maxAngle = 0;
		for(int j = 0; j < 3; j++){ // Over dW
			// Calculate the cylindrical angle.
			thetaCyl = pos->theta;
			rprime = std::sqrt(pos->r0*pos->r0 + dW[j]*dW[j]);
			alpha = std::atan2(dW[j], pos->r0);
			addAngles(thetaCyl, alpha);

			// Calculate the X and Z axis components.
			x = rprime*std::sin(thetaCyl); // m
			z = rprime*std::cos(thetaCyl); // m

			for(int k = 0; k < 3; k++){ // Over dY
				r = std::sqrt(x*x + dY[k]*dY[k] + z*z);
				theta = std::acos(z/r)*180/pi;
	
				if(theta < minAngle) minAngle = theta;
				if(theta > maxAngle) maxAngle = theta;
			}
		}

		ofile << i << "\t" << minAngle << "\t" << maxAngle << std::endl;

		if(minAngle < overallMin) overallMin = minAngle;
		if(maxAngle > overallMax) overallMax = maxAngle;

		minima.push_back(minAngle);
		maxima.push_back(maxAngle);
	}		

	ofile.close();

	int nBins = (ceil(overallMax)-floor(overallMin))*10;
	TH1F *h = new TH1F("h", "h", nBins, floor(overallMin), ceil(overallMax));

	std::cout << " nBins=" << nBins << std::endl;

	std::cout << " Min Angle: " << floor(overallMin) << std::endl;
	std::cout << " Max Angle: " << ceil(overallMax) << std::endl;

	int count;
	double binCenter;
	for(int i = 1; i <= nBins; i++){
		count = 0;
		binCenter = h->GetBinCenter(i);
		for(size_t j = 0; j < minima.size(); j++){
			if(binCenter >= minima.at(j) && binCenter < maxima.at(j)) count++;
		}
		h->SetBinContent(i, count);
	}

	openCanvas1();
	h->Draw();
	this->wait();	

	return 0;
}

int main(int argc, char *argv[]){
	angleAnalyzer obj;

	return obj.execute(argc, argv);
}
