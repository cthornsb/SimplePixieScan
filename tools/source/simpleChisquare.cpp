#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <cmath>
#include <limits>

#include "TF1.h"
#include "TFile.h"
#include "TGraphErrors.h"

#include "simpleChisquare.hpp"

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

double comp2(double *x, double *p){
	for(size_t i = 1; i < Nelements; i++){
		if(xval[i-1] <= x[0] && x[0] < xval[i]){
			return p[0]*(yval[i-1]+(x[0]-xval[i-1])*(yval[i]-yval[i-1])/(xval[i]-xval[i-1])) + p[1];
		}
	}
	return -1;
}

chisquare::chisquare() : simpleTool(), initialParameter(1), fitRangeLow(-1), fitRangeHigh(-1), parLimitLow(-1), parLimitHigh(-1), userFitRange(false), userParLimits(false), addConstTerm(false) { }

chisquare::~chisquare(){
}

void chisquare::setFitRange(const double &low_, const double &high_){
	fitRangeLow = low_;
	fitRangeHigh = high_;
}

void chisquare::setParLimits(const double &low_, const double &high_){
	parLimitLow = low_;
	parLimitHigh = high_;
}

void chisquare::getFitRange(double &low_, double &high_){
	low_ = fitRangeLow;
	high_ = fitRangeHigh;
}

void chisquare::getParLimits(double &low_, double &high_){
	low_ = parLimitLow;
	high_ = parLimitHigh;
}

void chisquare::addOptions(){
	addOption(optionExt("initial", required_argument, NULL, 0x0, "<parameter>", "Initial scaling factor (default=1)."), userOpts, optstr);
	addOption(optionExt("file-a", required_argument, NULL, 'a', "<fname:gname>", "Specify THEORETICAL filename and graph name."), userOpts, optstr);
	addOption(optionExt("file-b", required_argument, NULL, 'b', "<fname:gname>", "Specify EXPERIMENTAL filename and graph name."), userOpts, optstr);
	addOption(optionExt("fit-range", required_argument, NULL, 0x0, "<low:high>", "Specify fitting range to use (default uses bounds of THEORY graph)."), userOpts, optstr);
	addOption(optionExt("par-limits", required_argument, NULL, 0x0, "<low:high>", "Specify parameter limits for scaling factor A (not used by default)."), userOpts, optstr);
	addOption(optionExt("add-constant", no_argument, NULL, 0x0, "", "Add an additional constant term to the distribution."), userOpts, optstr);
}

bool chisquare::processArgs(){
	if(userOpts.at(0).active){
		initialParameter = strtod(userOpts.at(0).argument.c_str(), NULL);
	}
	if(userOpts.at(1).active){
		if(!splitByColon(userOpts.at(1).argument, fnames[0], gnames[0])){
			fnames[0] = userOpts.at(1).argument;
			gnames[0] = "graph";
		}
	}
	else{
		std::cout << " Error: THEORETICAL filename not specified!\n";
		return false;
	}
	if(userOpts.at(2).active){
		if(!splitByColon(userOpts.at(2).argument, fnames[1], gnames[1])){
			fnames[1] = userOpts.at(2).argument;
			gnames[1] = "graph";
		}
	}
	else{
		std::cout << " Error: EXPERIMENTAL filename not specified!\n";
		return false;
	}
	if(userOpts.at(3).active){
		userFitRange = true;
		if(!splitByColon(userOpts.at(3).argument, fitRangeLow, fitRangeHigh)){
			std::cout << " Error: Fit range must be specified as \"low:high\"!\n";
			return false;
		}
	}
	if(userOpts.at(4).active){
		userParLimits = true;
		if(!splitByColon(userOpts.at(4).argument, parLimitLow, parLimitHigh)){
			std::cout << " Error: Fit range must be specified as \"low:high\"!\n";
			return false;
		}
	}
	if(userOpts.at(5).active){
		addConstTerm = true;
	}

	return true;
}

int chisquare::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	return (this->compute() ? 0 : 1);
}

bool chisquare::compute(){
	TGraphErrors *ptr = NULL;
	TGraphErrors *graphs[2] = {NULL, NULL};
	for(size_t i = 0; i < 2; i++){
		TFile *file = new TFile(fnames[i].c_str(), "READ");
		if(!file->IsOpen()){
			std::cout << " Error! Failed to load input file \"" << fnames[i] << "\".\n";
			return false;
		}
		file->GetObject(gnames[i].c_str(), ptr);
		if(!ptr){
			std::cout << " Error! Failed to load \"" << gnames[i] << "\" from input file \"" << fnames[i] << "\".\n";
			file->Close();
			return false;
		}
		graphs[i] = (TGraphErrors*)ptr->Clone(gnames[i].c_str());
		file->Close();
		delete file;
	}

	xval = graphs[0]->GetX();
	yval = graphs[0]->GetY();
	Nelements = graphs[0]->GetN();

	std::cout << "THEORETICAL=" << graphs[0]->GetN() << " points, EXPERIMENTAL=" << graphs[1]->GetN() << " points\n";

	if(graphs[0]->GetN() == 0){
		std::cout << " Error! THEORETICAL graph is empty!\n";
		return false;
	}
	
	if(graphs[1]->GetN() == 0){
		std::cout << " Error! EXPERIMENTAL graph is empty!\n";
		return false;
	}

	if(!userFitRange){
		fitRangeLow = std::numeric_limits<double>::max();
		fitRangeHigh = std::numeric_limits<double>::min();
		for(size_t i = 0; i < Nelements; i++){
			if(xval[i] < fitRangeLow) fitRangeLow = xval[i];
			if(xval[i] > fitRangeHigh) fitRangeHigh = xval[i];
		}
	}
	
	std::cout << " Setting fit range to [" << fitRangeLow << ", " << fitRangeHigh << "]\n";
	TF1 *func;
	if(!addConstTerm)
		func = new TF1("func", comp, fitRangeLow, fitRangeHigh, 1);
	else{
		func = new TF1("func", comp2, fitRangeLow, fitRangeHigh, 2);
		func->SetParameter(1, 0);
		func->SetParLimits(1, 0, 100); // Limit the constant term.
	}
	func->SetParameter(0, initialParameter);
	
	if(userParLimits){
		std::cout << " Setting limits on A to [" << parLimitLow << ", " << parLimitHigh << "]\n";
		func->SetParLimits(0, parLimitLow, parLimitHigh);
	}
	
	std::cout << " Minimizing: (A x gTHEORY) = gEXP\n\n";
	
	graphs[1]->Fit(func, "R");
	double A = func->GetParameter(0);
	double B = (addConstTerm ? func->GetParameter(1) : -1);

	// Print the results.
	std::cout << "\nResults:\n A=" << A << " +/- " << func->GetParError(0);
	if(addConstTerm)
		std::cout << ", B=" << B << " +/- " << func->GetParError(1);
	std::cout << ", chi2=" << func->GetChisquare()/func->GetNDF() << std::endl;

	// Append the results to the ascii file.
	if(!output_filename.empty()){
		std::string prefix, suffix;
		if(!splitFilename(output_filename, prefix, suffix))
			prefix = output_filename;

		TFile *fout = new TFile((prefix+".root").c_str(), "UPDATE");
		std::ofstream asciiOut((prefix+".dat").c_str(), std::ios_base::app);

		asciiOut << gnames[0] << "\t" << gnames[1] << "\t" << A << "\t" << func->GetParError(0);
		if(addConstTerm)
			asciiOut << "\t" << B << "\t" << func->GetParError(1);
		asciiOut << "\t" << func->GetChisquare()/func->GetNDF() << std::endl;
	
		TGraph *goutput = new TGraph(Nelements);
		for(size_t i = 0; i < Nelements; i++){
			if(!addConstTerm)
				goutput->SetPoint(i, xval[i], A*yval[i]);
			else
				goutput->SetPoint(i, xval[i], A*yval[i]+B);
		}
	
		fout->cd();
		goutput->Write(gnames[0].c_str());
		fout->Close();
	
		delete fout;

		asciiOut.close();
	}
	
	delete func;
			
	return true;
}
