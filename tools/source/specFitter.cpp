#include <iostream>
#include <vector>

#include "TSpectrum.h"
#include "TMarker.h"
#include "TCanvas.h"
#include "TTree.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"

#include "simpleTool.hpp"

class specFitter : public simpleHistoFitter {
  private:
	double Xmin, Xmax;
	double Xrange;
	int nBins;

  public:
	specFitter() : simpleHistoFitter() { }

	bool fitSpectrum(TH1 *h_, std::ofstream &f_);

	bool process();
};

bool specFitter::fitSpectrum(TH1 *h_, std::ofstream &f_){
	if(!h_) return false; 

	h_->Draw();
	can2->Update();

	int numPeaks;
	std::cout << " How many peaks? "; std::cin >> numPeaks;
	
	if(numPeaks <= 0) 
		return false;

	/*TSpectrum spec(numPeaks);
	spec.Search(h_);
	
	// Print the list of peaks which were found.
	if(debug) std::cout << " Found peaks at the following locations -\n";
	for(int i = 0; i < numPeaks; i++){
		if(debug) std::cout << "  " << i << ": " << spec.GetPositionX()[i] << " ns\n";
		stream << "+gaus(" << 3*i+1 << ")";
	}*/

	// Build up the formula string.
	std::stringstream stream; stream << "[0]+x*[1]";
	for(int i = 0; i < numPeaks; i++){
		stream << "+gaus(" << 3*i+2 << ")";
	}

	// Define the total fit function.
	if(debug) std::cout << " Declaring function \"" << stream.str() << "\".\n";
	TF1 *func = new TF1("func", stream.str().c_str(), Xmin, Xmax);

	double amplitude;
	double meanValue;
	double lowMark;
	double highMark;
	
	// Set initial function parameters.
	TMarker *marker = (TMarker*)can2->WaitPrimitive("TMarker");
	func->SetParameter(0, marker->GetY());
	delete marker;
	func->SetParameter(1, 0);
	for(int i = 0; i < numPeaks; i++){
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		lowMark = marker->GetX();
		delete marker;
		marker = (TMarker*)can2->WaitPrimitive("TMarker");
		highMark = marker->GetX();
		delete marker;
		amplitude = getMaximum(h_, lowMark, highMark, meanValue);
		func->SetParameter(3*i+2, amplitude);
		func->SetParameter(3*i+3, meanValue);
		func->SetParameter(3*i+4, (highMark-lowMark)/2.354820);
	}
	
	// Fit the spectrum.
	h_->Fit(func, "QN0");

	// Print the fit results.
	if(debug){
		std::cout << "\nFit results:\n";
		std::cout << " Reduced chi^2 = " << func->GetChisquare()/func->GetNDF() << std::endl;
	}
	for(int i = 0; i < 3*numPeaks+2; i++){
		if(debug) std::cout << " p[" << i << "] = " << func->GetParameter(i) << std::endl;
		f_ << "\t" << func->GetParameter(i);
	}
	f_ << std::endl;

	// Draw the total fit.
	func->Draw("SAME");

	// Draw the composite gaussians.
	double totalIntegral = Xrange*func->GetParameter(0);
	if(debug){
		std::cout << "\nIntegrals:\n";
		std::cout << " background: " << totalIntegral << " (" << totalIntegral*(nBins/Xrange) << " counts)\n";
	}
	TF1 **lilfuncs = new TF1*[numPeaks];
	for(int i = 0; i < numPeaks; i++){
		lilfuncs[i] = new TF1("name", "gaus", Xmin, Xmax);
		lilfuncs[i]->SetLineColor(kGreen+3);
		lilfuncs[i]->SetParameter(0, func->GetParameter(3*i+2));
		lilfuncs[i]->SetParameter(1, func->GetParameter(3*i+3));
		lilfuncs[i]->SetParameter(2, func->GetParameter(3*i+4));
		lilfuncs[i]->Draw("SAME");
		
		if(debug){
			double temp = lilfuncs[i]->Integral(Xmin, Xmax);
			totalIntegral += temp;
			std::cout << " peak " << i << ": " << temp << " (" << temp*(nBins/Xrange) << " counts)\n";
		}
	}
	can2->Update();

	if(debug){
		std::cout << " total: " << totalIntegral << " (" << totalIntegral*(nBins/Xrange) << " counts)\n";
		std::cout << " hist: " << h_->Integral() << std::endl;
		std::cout << " diff: " << (1 - totalIntegral*(nBins/Xrange)/h_->Integral())*100 << " %\n";
	}

	return true;
}

bool specFitter::process(){
	if(!h2d || !can2) return false;
	
	std::ofstream ofile(output_filename.c_str());
	ofile << "id\tp0\tp1\tA0\tmu0\tsig0\tA1\tmu1\tsig1\tA2\tmu2\tsig2\tchi2\n";

	nBins = h2d->GetXaxis()->GetNbins();
	Xmin = h2d->GetXaxis()->GetXmin();
	Xmax = h2d->GetXaxis()->GetXmax();
	Xrange = Xmax-Xmin;

	TH1D *h1 = new TH1D("h1", "", nBins, Xmin, Xmax);
	h1->SetStats(0);

	can2->cd();

	for(int i = 1; i <= h2d->GetYaxis()->GetNbins(); i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(getProjectionX(h1, h2d, i)){ 
			std::cout << "DONE\n";

			std::stringstream title;
			title << "Bin " << i << " [" << h2d->GetYaxis()->GetBinLowEdge(i) << ", " << h2d->GetYaxis()->GetBinLowEdge(i)+h2d->GetYaxis()->GetBinWidth(i) << "]";

			h1->GetYaxis()->SetRangeUser(0, 1.1*h1->GetBinContent(h1->GetMaximumBin()));
			h1->SetTitle(title.str().c_str());

			ofile << i;
			if(fitSpectrum(h1, ofile))
				can2->WaitPrimitive();
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
