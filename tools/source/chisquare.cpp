#include <iostream>
#include <vector>
#include <time.h>
#include <cmath>
#include <limits>

#include "TF1.h"
#include "TFile.h"
#include "TGraphErrors.h"

#include "simpleTool.hpp"

// These need to be global.
size_t Nelements=0;
double *xval, *yval;

double comp(double *x, double *p){
	for(size_t i = 1; i < Nelements; i++){
		if(xval[i-1] <= x[0] && x[0] < xval[i]){
			return p[0]*(yval[i-1]+(x[0]-xval[i-1])*(yval[i]-yval[i-1])/(xval[i]-xval[i-1]));
		}
	}
	return -1;
}

class chisquare : public simpleTool {
  private:
	double initialParameter;
	double fitRangeLow;
	double fitRangeHigh;
	
	bool userFitRange;
	bool userParLimits;
	
	std::string fnames[2];
	std::string gnames[2];

  public:
	chisquare() : simpleTool(), initialParameter(1), fitRangeLow(-1), fitRangeHigh(-1), userFitRange(false), userParLimits(false) { }

	~chisquare();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

chisquare::~chisquare(){
}

void chisquare::addOptions(){
	addOption(optionExt("initial", required_argument, NULL, 0x0, "<initialParameter>", "Initial scaling factor (default=1)."), userOpts, optstr);
	addOption(optionExt("file-a", required_argument, NULL, 'a', "<fname:gname>", "Specify THEORETICAL filename and graph name."), userOpts, optstr);
	addOption(optionExt("file-b", required_argument, NULL, 'b', "<fname:gname>", "Specify EXPERIMENTAL filename and graph name."), userOpts, optstr);
}

bool chisquare::processArgs(){
	if(userOpts.at(0).active){
		initialParameter = strtod(userOpts.at(0).argument.c_str(), NULL);
	}
	if(userOpts.at(1).active){
		size_t index = userOpts.at(1).argument.find(':');
		if(index != std::string::npos){
			fnames[0] = userOpts.at(1).argument.substr(0, index);
			gnames[0] = userOpts.at(1).argument.substr(index+1);
		}
		else{
			fnames[0] = userOpts.at(1).argument;
			gnames[0] = "graph";
		}
	}
	else{
		std::cout << " Error: THEORETICAL filename not specified!\n";
		return false;
	}
	if(userOpts.at(2).active){
		size_t index = userOpts.at(2).argument.find(':');
		if(index != std::string::npos){
			fnames[1] = userOpts.at(2).argument.substr(0, index);
			gnames[1] = userOpts.at(2).argument.substr(index+1);
		}
		else{
			fnames[1] = userOpts.at(2).argument;
			gnames[1] = "graph";
		}
	}
	else{
		std::cout << " Error: EXPERIMENTAL filename not specified!\n";
		return false;
	}

	return true;
}

int chisquare::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	/*if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 2;
	}*/

	TGraphErrors *ptr = NULL;
	TGraphErrors *graphs[2] = {NULL, NULL};
	for(size_t i = 0; i < 2; i++){
		//std::cout << "debug: getting \"" << 
		TFile *file = new TFile(fnames[i].c_str(), "READ");
		if(!file->IsOpen()){
			std::cout << " Error! Failed to load input file \"" << fnames[i] << "\".\n";
			return 2;
		}
		file->GetObject(gnames[i].c_str(), ptr);
		if(!ptr){
			std::cout << " Error! Failed to load \"" << gnames[i] << "\" from input file \"" << fnames[i] << "\".\n";
			file->Close();
			return 3;
		}
		graphs[i] = (TGraphErrors*)ptr->Clone(gnames[i].c_str());
		file->Close();
		delete file;
	}

	xval = graphs[0]->GetX();
	yval = graphs[0]->GetY();
	Nelements = graphs[0]->GetN();

	std::cout << "THEORETICAL=" << graphs[0]->GetN() << " points, EXPERIMENTAL=" << graphs[1]->GetN() << " points\n";
	std::cout << " Minimizing: A x gTHEORY = gEXP\n\n";

	/*std::string outputPrefix(argv[4]);
	TFile *fout = new TFile((outputPrefix+".root").c_str(), "UPDATE");
	std::ofstream asciiOut((outputPrefix+".dat").c_str(), std::ios_base::app);*/

	if(!userFitRange){
		fitRangeLow = std::numeric_limits<double>::max();
		fitRangeHigh = std::numeric_limits<double>::min();
		for(size_t i = 0; i < Nelements; i++){
			if(xval[i] < fitRangeLow) fitRangeLow = xval[i];
			if(xval[i] > fitRangeHigh) fitRangeHigh = xval[i];
		}
	}
	
	std::cout << " Setting fit range to [" << fitRangeLow << ", " << fitRangeHigh << "]\n";
	TF1 *func = new TF1("func", comp, fitRangeLow, fitRangeHigh, 1);
	func->SetParameter(0, initialParameter);
	
	if(userParLimits){
		double parLimitLow;
		double parLimitHigh;
		std::cout << " Setting limits on A to [" << parLimitLow << ", " << parLimitHigh << "]\n";
		func->SetParLimits(0, parLimitLow, parLimitHigh);
	}
	
	graphs[1]->Fit(func, "R");
	double A = func->GetParameter(0);

	std::cout << "\nResults:\n A=" << A << " +/- " << func->GetParError(0) << ", chi2=" << func->GetChisquare()/func->GetNDF() << std::endl;
	/*asciiOut << energyStr << "\t" << A << "\t" << func->GetParError(0) << "\t" << func->GetChisquare()/func->GetNDF() << std::endl;
	
	TGraph *goutput = new TGraph(Nelements);
	for(size_t i = 0; i < Nelements; i++){
		goutput->SetPoint(i, xval[i], A*yval[i]);
	}
	
	fout->cd();
	goutput->Write(("g"+energyStr).c_str());
	fout->Close();
	
	f1->Close();
	f2->Close();
	
	delete func;
	delete fout;
	delete f1;
	delete f2;

	asciiOut.close();

	std::cout << "\n\n Done! Wrote " << outtree->GetEntries() << " entries to '" << output_filename << "'.\n";*/
	
	delete func;
			
	return 0;
}

int main(int argc, char *argv[]){
	chisquare obj;

	return obj.execute(argc, argv);
}
