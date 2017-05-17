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

double integrateHist(TH1 *h, const double &low, const double &high){
	int lowBin = h->FindBin(low);
	int highBin = h->FindBin(high);
	if(highBin > h->GetNbinsX())
		highBin = h->GetNbinsX();
	double retval = 0;
	for(int i = lowBin; i <= highBin; i++)
		retval += h->GetBinContent(i);
	return retval;
}

void calculateP2(double *x, double *y, double *p){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		x1[i] = x[i];
		x2[i] = std::pow(x[i], 2);
	}

	double denom = (x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(x2[2]-x2[1]*1) + x2[0]*(x1[2]-x1[1]*1);

	p[0] = (float)((y[0]*(x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(y[1]*x2[2]-x2[1]*y[2]) + x2[0]*(y[1]*x1[2]-x1[1]*y[2]))/denom);
	p[1] = (float)(((y[1]*x2[2]-x2[1]*y[2]) - y[0]*(x2[2]-x2[1]*1) + x2[0]*(y[2]-y[1]*1))/denom);
	p[2] = (float)(((x1[1]*y[2]-y[1]*x1[2]) - x1[0]*(y[2]-y[1]*1) + y[0]*(x1[2]-x1[1]*1))/denom);
}

class specFitter : public simpleHistoFitter {
  private:
	double Xmin, Xmax;
	double Xrange;
	int nBins;

	bool polyBG;
	bool gausFit;

  public:
	specFitter() : simpleHistoFitter(), polyBG(false), gausFit(true) { }

	bool fitSpectrum(TH1 *h_, std::ofstream &f_, const int &binID_);

	void addChildOptions();
	
	bool processChildArgs();

	bool process();
};

bool specFitter::fitSpectrum(TH1 *h_, std::ofstream &f_, const int &binID_){
	if(!h_) return false; 

	h_->Draw();
	can2->Update();

	TMarker *marker;

	double xlo = h_->GetXaxis()->GetXmin();
	double ylo = h_->GetYaxis()->GetXmin();
	double xhi, yhi;

	std::cout << " Mark top-right bounding point (xmax,ymax)\n";
	marker = (TMarker*)can2->WaitPrimitive("TMarker");
	xhi = marker->GetX();
	yhi = marker->GetY();
	h_->GetXaxis()->SetRangeUser(xlo, xhi);
	h_->GetYaxis()->SetRangeUser(ylo, yhi);
	h_->Draw();
	can2->Update();

	int numPeaks = 1;
	//std::cout << " How many peaks? "; std::cin >> numPeaks;
	
	if(numPeaks <= 0) 
		return false;

	double amplitude;
	double meanValue;
	
	double bgx1, bgy1;
	double bgx2, bgy2;

	double bgx[3];
	double bgy[3];
	double p[3];

	int nPars = 3*numPeaks;

	std::stringstream stream;

	if(!polyBG){
		nPars += 3;

		// Set initial function parameters.
		std::cout << "Mark exponential background (x0,y0)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		bgx1 = marker->GetX();
		bgy1 = marker->GetY();
		delete marker;
		std::cout << "Mark exponential background (x1,y1)\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");	
		bgx2 = marker->GetX();
		bgy2 = marker->GetY();
		delete marker;

		p[0] = bgy2*0.9;
		p[2] = std::log((bgy1-p[0])/(bgy2-p[0]))/(bgx1-bgx2);
		p[1] = std::log(bgy1)-p[2]*bgx1;

		// Build up the formula string.
		stream << "[0]+expo(1)";
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

		calculateP2(bgx, bgy, p);

		bgx1 = bgx[0];
		bgx2 = bgx[2];

		// Build up the formula string.
		stream << "[0]+x*[1]+x^2*[2]";
	}

	if(debug) std::cout << " p0 = " << p[0] << ", p1 = " << p[1] << ", p2 = " << p[2] << std::endl;

	for(int i = 0; i < numPeaks; i++){
		if(gausFit) stream << "+gaus(" << 3*i+3 << ")";
		else stream << "+landau(" << 3*i+3 << ")";
	}

	// Define the total fit function.
	if(debug) std::cout << " Declaring function \"" << stream.str() << "\".\n";
	TF1 *func = new TF1("func", stream.str().c_str(), bgx1, bgx2);

	func->SetParameter(0, p[0]);
	func->SetParameter(1, p[1]);
	func->SetParameter(2, p[2]);

	double bkgA, fwhmLeft, fwhmRight;
	for(int i = 0; i < numPeaks; i++){
		std::cout << "Mark peak " << i+1 << " maximum\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		amplitude = marker->GetY();
		meanValue = marker->GetX();
		if(!polyBG)
			bkgA = p[0]+std::exp(p[1]+p[2]*meanValue);
		else
			bkgA = p[0]+p[1]*meanValue+p[2]*meanValue*meanValue;
		amplitude = amplitude-bkgA;
		delete marker;
		TLine line(xlo, amplitude*0.5+bkgA, xhi, amplitude*0.5+bkgA);
		line.Draw("SAME");
		can2->Update();
		std::cout << "Mark peak " << i+1 << " left and right\n";
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		fwhmLeft = marker->GetX();
		delete marker;
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		fwhmRight = marker->GetX();
		delete marker;
		if(gausFit) func->SetParameter(3*i+3, amplitude);
		else func->SetParameter(3*i+3, amplitude*5.9);
		func->SetParameter(3*i+4, meanValue);
		func->SetParameter(3*i+5, (fwhmRight-fwhmLeft)/2.35);
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
	if(!polyBG)
		bkgfunc = new TF1("bkgfunc", "[0]+expo(1)", bgx1, bgx2);
	else
		bkgfunc = new TF1("bkgfunc", "[0]+x*[1]+x^2*[2]", bgx1, bgx2);
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
	double integral = bkgfunc->Integral(bgx1, bgx2);
	double binWidth = h_->GetBinWidth(1);
	totalIntegral += integral;
	if(debug){
		std::cout << " Integrals:\n";
		std::cout << "  background: " << integral << " (" << integral/binWidth << " counts)\n";
	}
	f_ << "\t" << integral/binWidth; // Write the background fit integral in fit range.
	TF1 **lilfuncs = new TF1*[numPeaks+1];
	for(int i = 0; i < numPeaks; i++){
		if(gausFit) lilfuncs[i] = new TF1("name", "gaus", bgx1, bgx2);
		else lilfuncs[i] = new TF1("name", "landau", bgx1, bgx2);
		lilfuncs[i]->SetLineColor(kGreen+3);
		lilfuncs[i]->SetParameter(0, func->GetParameter(3*i+3));
		lilfuncs[i]->SetParameter(1, func->GetParameter(3*i+4));
		lilfuncs[i]->SetParameter(2, func->GetParameter(3*i+5));
		lilfuncs[i]->Draw("SAME");
	}
	can2->Update();
	can2->WaitPrimitive();

	double histIntegral = integrateHist(h_, bgx1, bgx2);
	if(debug){
		std::cout << "  total: " << totalIntegral << " (" << totalIntegral/binWidth << " counts)\n";
		std::cout << "  hist: " << integrateHist(h_, bgx1, bgx2) << " counts\n";
	}
	f_ << "\t" << histIntegral << "\n"; // Write the histogram counts in fit range.

	for(int i = 0; i < numPeaks; i++){ // Write the individual peak fit results to file.
		integral = lilfuncs[i]->Integral(bgx1, bgx2);
		totalIntegral += integral;
	
		f_ << binID_ << "\t" << i << "\t" << !gausFit << "\t";
		f_ << func->GetParameter(3*i+3) << "\t" << func->GetParError(3*i+3) << "\t"; 
		f_ << func->GetParameter(3*i+4) << "\t" << func->GetParError(3*i+4) << "\t"; 
		f_ << func->GetParameter(3*i+5) << "\t" << func->GetParError(3*i+5) << "\t"; 

		if(debug) std::cout << "  peak " << i << ": " << integral << " (" << integral/binWidth << " counts)\n";
		f_ << integral/binWidth << "\n";
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
}

bool specFitter::processChildArgs(){
	if(userOpts.at(firstChildOption).active)
		polyBG = true;
	if(userOpts.at(firstChildOption+1).active)
		gausFit = false;

	return true;
}

bool specFitter::process(){
	if(!h2d || !can2) return false;

	std::ofstream ofile;
	if(!output_filename.empty())	
		ofile.open(output_filename.c_str());
	else
		ofile.open("specFitter.out");
	ofile << "binID\tbinLow\tchi2\tp0\tp0err\tp1\tp1err\tp2\tp2err\tIbkg\thistCounts\n";
	ofile << "binID\tpeakID\tfunction\tA\tAerr\tmu\tmuerr\tsigma\tsigmaerr\tIpeak\n";

	TH1D *h1 = getProjectionHist(h2d);
	h1->SetStats(0);

	nBins = h1->GetNbinsX();
	Xmin = h1->GetXaxis()->GetXmin();
	Xmax = h1->GetXaxis()->GetXmax();
	Xrange = Xmax-Xmin;

	can2->cd();

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
			fitSpectrum(h1, ofile, i);
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
