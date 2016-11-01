#include <iostream>
#include <sstream>
#include <fstream>

#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TH1D.h"
#include "TMarker.h"
#include "TMath.h"

#include "simpleTool.hpp"

class calibrator : public simpleHistoFitter {
  public:
	calibrator() : simpleHistoFitter() { }

	bool process();
};

bool calibrator::process(){
	if(!h2d || !can2) return false;

	int fitMode = -1;
	std::string userInput = "";
	while(fitMode < 0){
		std::cout << "Gaussian or compton edge calibration? (g|c): ";
		std::cin >> userInput;
		if(userInput == "g")
			fitMode = 0;
		else if(userInput == "c")
			fitMode = 1;
		else
			std::cout << " ERROR: Invalid selection. Choose 'g' or 'c'.\n";
	}

	std::ofstream ofile("fitresults.dat");
	
	can2->cd()->SetLogy();
	
	TH1D *h1 = new TH1D("h1", "", h2d->GetXaxis()->GetNbins(), h2d->GetXaxis()->GetXmin(), h2d->GetXaxis()->GetXmax());

	TF1 *f1 = NULL;
	int numPeaks = -1;
	if(fitMode == 0){
		ofile << "id\tA\tmean\tsigma\tchi2\n";
		std::cout << "Using gaussian photo peak fitting.\n";
		while(numPeaks < 0){
			std::cout << "How many peaks in spectrum? ";
			std::cin >> numPeaks;
			if(numPeaks <= 0)
				std::cout << " ERROR: Invalid specification. Must be non-zero.\n";
		}
		f1 = new TF1("f1", "gaus", 0, 1);
	}
	else if(fitMode == 1){
		ofile << "id\tp0\tp1\tavg\tedge\tchi2\n";
		std::cout << "Using compton edge neutron spectrum fitting.\n";
		f1 = new TF1("f1", "expo", 0, 1);
		numPeaks = 1;
	}

	TFitResultPtr fitResult;
	TMarker *m1;
				
	double average, edge;
	double lowbin, highbin;
	double x1, x2, x3, x4;

	for(int i = 1; i <= h2d->GetYaxis()->GetNbins(); i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(getProjectionX(h1, h2d, i)){ 
			std::cout << "DONE\n";

			std::stringstream stream; stream << h2d->GetYaxis()->GetBinLowEdge(i);
			h1->SetTitle(stream.str().c_str());
			
			can2->Clear();
			h1->Draw();
			can2->Update();
			
			for(int j = 0; j < numPeaks; j++){
				m1 = (TMarker*)can2->WaitPrimitive("TMarker");
				x1 = m1->GetX();
				m1->Delete();
				m1 = (TMarker*)can2->WaitPrimitive("TMarker");
				x2 = m1->GetX();
				m1->Delete();
			
				if(fitMode == 0){
					std::cout << "  Range: " << x1 << ", " << x2 << std::endl;
					f1->SetRange(x1, x2);
				}
				else if(fitMode == 1){
					m1 = (TMarker*)can2->WaitPrimitive("TMarker");
					x3 = m1->GetX();
					m1->Delete();
					m1 = (TMarker*)can2->WaitPrimitive("TMarker");
					x4 = m1->GetX();
					m1->Delete();
				
					std::cout << "  Averaging range: " << x1 << ", " << x2 << std::endl;
					std::cout << "  Fitting range: " << x3 << ", " << x4 << std::endl;
					f1->SetRange(x3, x4);
				}
			
				h1->Fit(f1, "QR");
				f1->Draw("SAME");
			
				// Output the fit results.
				std::cout << "  Fit: chi^2 = " << f1->GetChisquare()/f1->GetNDF();
				if(fitMode == 0){
					std::cout << ", t0 = " << f1->GetParameter(1) << " ns\n";
					ofile << stream.str() << "\t" << f1->GetParameter(0) << "\t" << f1->GetParameter(1) << "\t" << f1->GetParameter(2) << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;			
				}
				else if(fitMode == 1){ // Calculate the edge.
					// Find the maximum.
					average = 0.0;
					lowbin = h1->FindBin(x1);
					highbin = h1->FindBin(x2);
					for(int i = lowbin; i <= highbin; i++){
						average += h1->GetBinContent(i);
					}
					average = average / (highbin-lowbin);
					edge = (TMath::Log(average/2.0) - f1->GetParameter(0)) / f1->GetParameter(1);
			
					std::cout << ", avg = " << average << ", edge = " << edge << " ADC bins\n";
					ofile << stream.str() << "\t" << f1->GetParameter(0) << "\t" << f1->GetParameter(1) << "\t" << average << "\t" << edge << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;	
				}
			}
			can2->WaitPrimitive();
		}
		else std::cout << "FAILED\n";
	}
	
	delete h1;
	delete f1;
	
	return true;
}

int main(int argc, char *argv[]){
	calibrator obj;
	
	return obj.execute(argc, argv);
}
