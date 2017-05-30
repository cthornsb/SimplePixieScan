#include <iostream>
#include <vector>

#include "TSpectrum.h"
#include "TMarker.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TTree.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"

#include "simpleTool.hpp"

// Return the range of bins containing an upper and lower point, rounded to the nearest bins.
void findBins(TH1 *h, const double &xstart_, const double &xstop_, int &lowBin, int &highBin){
	if(xstart_ <= h->GetXaxis()->GetXmin()) lowBin = 1;
	else                                    lowBin = h->FindBin(xstart_);
	if(xstop_ >= h->GetXaxis()->GetXmax())  highBin = h->GetNbinsX();
	else                                    highBin = h->FindBin(xstop_);
}

// Return the number of counts in a 1-d histogram in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts in a histogram, not the integral.
double summation(TH1 *h, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	for(int i = lowBin; i <= highBin; i++)
		retval += h->GetBinContent(i);
	return retval;
}

// Return the number of counts under a TF1 in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts under a curve, not the function integral.
double summation(TH1 *h, TF1 *f, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	double xlo, xhi;
	for(int i = lowBin; i <= highBin; i++){
		xlo = h->GetBinLowEdge(i);
		xhi = xlo + h->GetBinWidth(i);
		retval += f->Integral(xlo, xhi)/(xhi-xlo);
	}
	return retval;
}

void calculateP2(double *x, double *y, double *p, bool log=false){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		if(!log) x1[i] = x[i];
		else     x1[i] = std::log(x[i]);
		x2[i] = std::pow(x1[i], 2);
	}

	p[2] = (y[2]-y[0]+(y[0]-y[1])*(x1[2]-x1[0])/(x1[1]-x1[0]))/(x2[2]-x2[0]-(x2[1]-x2[0])*(x1[2]-x1[0])/(x1[1]-x1[0]));
	p[1] = (y[1]-y[0]-(x2[1]-x2[0])*p[2])/(x1[1]-x1[0]);
	p[0] = y[0] - p[1]*x1[0] - p[2]*x2[0];
}

class specFitter : public simpleHistoFitter {
  private:
	double Xmin, Xmax;
	double Xrange;
	int nBins;

	bool polyBG;
	bool gausFit;

	bool logXaxis;
	bool logYaxis;

  public:
	specFitter() : simpleHistoFitter(), polyBG(false), gausFit(true), logXaxis(false), logYaxis(false) { }

	bool fitSpectrum(TH1 *h_, std::ofstream &f_, const int &binID_, const bool &log_=false);

	void addChildOptions();
	
	bool processChildArgs();

	bool process();
};

bool specFitter::fitSpectrum(TH1 *h_, std::ofstream &f_, const int &binID_, const bool &log_/*=false*/){
	if(!h_) return false; 

	h_->Draw();
	can2->Update();

	TMarker *marker;

	double xlo, ylo;
	double xhi, yhi;
	
	std::cout << " Mark bottom-left bounding point (xmin,ymin)\n";
	marker = (TMarker*)can2->WaitPrimitive("TMarker");
	xlo = marker->GetX();
	ylo = marker->GetY();
	delete marker;	
	
	std::cout << " Mark top-right bounding point (xmax,ymax)\n";
	marker = (TMarker*)can2->WaitPrimitive("TMarker");
	xhi = marker->GetX();
	yhi = marker->GetY();
	delete marker;

	h_->GetXaxis()->SetRangeUser(xlo, xhi);
	h_->GetYaxis()->SetRangeUser(ylo, yhi);
	h_->Draw();
	can2->Update();

	std::string xstr = "x";
	if(log_) xstr = "log(x)";

	int numPeaks = 1;
	//std::cout << " How many peaks? "; std::cin >> numPeaks;
	
	if(numPeaks <= 0) 
		return false;

	double amplitude;
	double meanValue;
	
	double bgx[3];
	double bgy[3];
	double p[3];

	int nPars = 3*numPeaks;

	std::stringstream stream1, stream2;

	if(!polyBG){
		nPars += 3;
		// Set initial function parameters.
		std::cout << "Mark exponential background (x0,y0)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		bgx[0] = marker->GetX();
		bgy[0] = marker->GetY();
		delete marker;
		std::cout << "Mark exponential background (x1,y1)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");	
		bgx[1] = marker->GetX();
		bgy[1] = marker->GetY();
		delete marker;
		std::cout << "Mark constant (xinf,yinf)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		bgx[2] = marker->GetX();
		bgy[2] = marker->GetY();
		delete marker;

		p[0] = bgy[2];
		if(log_){
			p[2] = std::log((bgy[0]-p[0])/(bgy[1]-p[0]))/(bgx[0]-bgx[1]);
			p[1] = std::log(bgy[0])-p[2]*bgx[0];
		}
		else{
			p[2] = std::log((bgy[0]-p[0])/(bgy[1]-p[0]))/std::log(bgx[0]/bgx[1]);
			p[1] = std::log(bgy[1]-p[0])-p[2]*std::log(bgx[0]);
		}
		
		stream1 << "[0]+exp([1]+[2]*" << xstr << ")";
	}
	else{
		nPars += 3;

		// Set initial function parameters.
		std::cout << "Mark linear background (x0,y0)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		bgx[0] = marker->GetX();
		bgy[0] = marker->GetY();
		delete marker;
		std::cout << "Mark linear background (x1,y1)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");	
		bgx[1] = marker->GetX();
		bgy[1] = marker->GetY();
		delete marker;
		std::cout << "Mark linear background (x2,y2)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");	
		bgx[2] = marker->GetX();
		bgy[2] = marker->GetY();
		delete marker;

		if(!log_) calculateP2(bgx, bgy, p);
		else      calculateP2(bgx, bgy, p, true);
		
		stream1 << "[0]+" << xstr << "*[1]+" << xstr << "^2*[2]";
	}

	if(debug) std::cout << " p0 = " << p[0] << ", p1 = " << p[1] << ", p2 = " << p[2] << std::endl;

	for(int i = 0; i < numPeaks; i++){
		if(gausFit) stream2 << "+[" << 3*i+3 << "]*TMath::Gaus(" << xstr << ", [" << 3*i+4 << "], [" << 3*i+5 << "])";
		else        stream2 << "+[" << 3*i+3 << "]*TMath::Landau(" << xstr << ", [" << 3*i+4 << "], [" << 3*i+5 << "])";
	}

	std::string totalString = stream1.str() + stream2.str();

	// Define the total fit function.
	if(debug) std::cout << " Declaring function \"" << totalString << "\".\n";
	TF1 *func = new TF1("func", totalString.c_str(), xlo, xhi);

	func->SetParameter(0, p[0]);
	func->SetParameter(1, p[1]);
	func->SetParameter(2, p[2]);

	double bkgA, fwhmLeft, fwhmRight;
	for(int i = 0; i < numPeaks; i++){
		std::cout << "Mark peak " << i+1 << " maximum\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		amplitude = marker->GetY();
		meanValue = marker->GetX();
		if(!polyBG){
			if(!log_) bkgA = p[0]+std::exp(p[1]+p[2]*meanValue);
			else      bkgA = p[0]+std::exp(p[1])*std::pow(meanValue, p[2]);
		}
		else{
			if(!log_) bkgA = p[0]+p[1]*meanValue+p[2]*meanValue*meanValue;
			else      bkgA = p[0]+p[1]*std::log(meanValue)+p[2]*std::pow(std::log(meanValue), 2);
		}
		amplitude = amplitude-bkgA;
		delete marker;
		TLine line(xlo, amplitude*0.5+bkgA, xhi, amplitude*0.5+bkgA);
		line.Draw("SAME");
		can2->Update();
		std::cout << "Mark left and right of peak " << i+1 << "\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		fwhmLeft = marker->GetX();
		delete marker;
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		fwhmRight = marker->GetX();
		delete marker;
		if(gausFit) func->SetParameter(3*i+3, amplitude);
		else        func->SetParameter(3*i+3, amplitude*5.9);
		if(!log_){
			func->SetParameter(3*i+4, meanValue);
			func->SetParameter(3*i+5, (fwhmRight-fwhmLeft)/2.35);
		}
		else{
			func->SetParameter(3*i+4, std::log(meanValue));
			double sig1 = std::log(fwhmRight/meanValue)/std::sqrt(std::log(4)); // Right side of the maximum.
			double sig2 = std::log(meanValue/fwhmLeft)/std::sqrt(std::log(4)); // Left side of the maximum.
			func->SetParameter(3*i+5, (sig1+sig2)/2);
		}
	}

	if(debug){ // Print the initial conditions.
		func->Draw("SAME");
		can2->Update();
		can2->WaitPrimitive();

		std::cout << " Initial:\n";
		for(int i = 0; i < nPars; i++){
			std::cout << "  p[" << i << "] = " << func->GetParameter(i) << std::endl;
		}
	}
	
	// Fit the spectrum.
	h_->Fit(func, "QN0R");

	if(debug){ // Print the fit results.
		std::cout << " Final:\n";
		for(int i = 0; i < nPars; i++){
			std::cout << "  p[" << i << "] = " << func->GetParameter(i) << std::endl;
		}
	}

	// Report the chi^2 of the fit.
	std::cout << " Reduced chi^2 = " << func->GetChisquare()/func->GetNDF() << std::endl;
	f_ << func->GetChisquare()/func->GetNDF();

	// Draw the total fit.
	func->Draw("SAME");

	TF1 *bkgfunc = NULL;
	bkgfunc = new TF1("bkgfunc", stream1.str().c_str(), xlo, xhi);
	
	bkgfunc->SetLineColor(kMagenta+1);
	bkgfunc->SetParameter(0, func->GetParameter(0));
	bkgfunc->SetParameter(1, func->GetParameter(1));
	bkgfunc->SetParameter(2, func->GetParameter(2));
	bkgfunc->Draw("SAME");

	// Write the background fit results to file.
	for(int i = 0; i < 3; i++)
		f_ << "\t" << func->GetParameter(i) << "\t" << func->GetParError(i); 

	// Draw the composite gaussians.
	double totalIntegral = 0;
	double integral = summation(h_, bkgfunc, xlo, xhi);
	totalIntegral += integral;
	if(debug){
		std::cout << " Integrals:\n";
		std::cout << "  background: " << integral << " (" << integral << " counts)\n";
	}
	f_ << "\t" << integral; // Write the background fit integral in fit range.
	TF1 **lilfuncs = new TF1*[numPeaks+1];
	for(int i = 0; i < numPeaks; i++){
		lilfuncs[i] = new TF1("name", stream2.str().c_str(), xlo, xhi);
		lilfuncs[i]->SetLineColor(kGreen+3);
		lilfuncs[i]->SetParameter(0, func->GetParameter(3*i+3));
		lilfuncs[i]->SetParameter(1, func->GetParameter(3*i+4));
		lilfuncs[i]->SetParameter(2, func->GetParameter(3*i+5));
		lilfuncs[i]->Draw("SAME");
	}
	can2->Update();
	can2->WaitPrimitive();

	double histIntegral = summation(h_, xlo, xhi);
	if(debug){
		std::cout << "  total: " << totalIntegral << " (" << totalIntegral << " counts)\n";
		std::cout << "  hist: " << summation(h_, xlo, xhi) << " counts\n";
	}
	f_ << "\t" << histIntegral << "\n"; // Write the histogram counts in fit range.

	for(int i = 0; i < numPeaks; i++){ // Write the individual peak fit results to file.
		integral = summation(h_, lilfuncs[i], xlo, xhi);
		totalIntegral += integral;
	
		f_ << binID_ << "\t" << i << "\t";
		if(!log_) f_ << !gausFit << "\t";
		else f_ << (2 + ((int)!gausFit)) << "\t";
		f_ << func->GetParameter(3*i+3) << "\t" << func->GetParError(3*i+3) << "\t"; 
		f_ << func->GetParameter(3*i+4) << "\t" << func->GetParError(3*i+4) << "\t"; 
		f_ << func->GetParameter(3*i+5) << "\t" << func->GetParError(3*i+5) << "\t"; 

		if(debug) std::cout << "  peak " << i << ": " << integral << " (" << integral << " counts)\n";
		f_ << integral << "\n";
		delete lilfuncs[i];
	}
	delete[] lilfuncs;
	delete bkgfunc;
	delete func;

	return true;
}

void specFitter::addChildOptions(){
	addOption(optionExt("poly", no_argument, NULL, 'p', "", "Fit background spectrum with 2nd order polynomial."), userOpts, optstr);
	addOption(optionExt("landau", no_argument, NULL, 'l', "", "Fit peaks with landau distributions."), userOpts, optstr);
	addOption(optionExt("logx", no_argument, NULL, 0x0, "", "Log the x-axis of the projection histogram."), userOpts, optstr);
	addOption(optionExt("logy", no_argument, NULL, 0x0, "", "Log the y-axis of the projection histogram."), userOpts, optstr);
}

bool specFitter::processChildArgs(){
	if(userOpts.at(firstChildOption).active)
		polyBG = true;
	if(userOpts.at(firstChildOption+1).active)
		gausFit = false;
	if(userOpts.at(firstChildOption+2).active)
		logXaxis = true;
	if(userOpts.at(firstChildOption+3).active)
		logYaxis = true;

	return true;
}

bool specFitter::process(){
	if(!h2d || !can2) return false;

	std::ofstream ofile;
	if(!output_filename.empty())	
		ofile.open(output_filename.c_str());
	else
		ofile.open("specFitter.out");

	ofile << "# Input root file\n";
	ofile << full_input_filename << ":" << input_objname << std::endl;
	ofile << "# binID\tbinLow\tchi2\tp0\tp0err\tp1\tp1err\tp2\tp2err\tIbkg\thistCounts\n";
	ofile << "# binID\tpeakID\tfunction\tA\tAerr\tmu\tmuerr\tsigma\tsigmaerr\tIpeak\n";

	TH1D *h1 = getProjectionHist(h2d);
	h1->SetStats(0);

	nBins = h1->GetNbinsX();
	Xmin = h1->GetXaxis()->GetXmin();
	Xmax = h1->GetXaxis()->GetXmax();
	Xrange = Xmax-Xmin;

	can2->cd();
	
	if(logXaxis){
		if(h1->GetXaxis()->GetXmin() <= 0){
			std::cout << " Warning! Failed to set x-axis to log scale (xmin=" << h1->GetXaxis()->GetXmin() << ").\n";
			logXaxis = false;
		}
		else{ can2->SetLogx(); }
	}
	if(logYaxis){
		if(h1->GetYaxis()->GetXmin() <= 0){
			std::cout << " Warning! Failed to set y-axis to log scale (ymin=" << h1->GetYaxis()->GetXmin() << ").\n";
			logYaxis = false;
		}
		else{ can2->SetLogy(); }
	}

	double binLow;
	int numProjections = getNumProjections(h2d);
	for(int i = 1; i <= numProjections; i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(getProjection(h1, h2d, i)){ 
			std::cout << "DONE\n";

			std::stringstream title;
			std::stringstream stream; 
			binLow = getBinLowEdge(h2d, i);
			stream << binLow;
			h1->SetTitle(stream.str().c_str());

			h1->GetXaxis()->SetRangeUser(Xmin, Xmax);
			h1->GetYaxis()->SetRangeUser(0, 1.1*h1->GetBinContent(h1->GetMaximumBin()));

			ofile << i << "\t" << binLow << "\t";
			fitSpectrum(h1, ofile, i, logXaxis);
		}
		else std::cout << "FAILED\n";
	}
	
	delete h1;

	ofile.close();
	
	return true;
}

int main(int argc, char *argv[]){
	specFitter obj;
	
	return obj.execute(argc, argv);
}
