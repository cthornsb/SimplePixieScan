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

#include "simpleTool.hpp"

class timeAlign : public simpleHistoFitter {
  private:
	bool scanArgs();

  public:
	timeAlign() : simpleHistoFitter() { }

	bool process();
};

bool timeAlign::process(){
	if(!h2d || !can2) return false;
	
	can2->cd()->SetLogy();
	
	TH1D *h1 = new TH1D("h1", "", h2d->GetXaxis()->GetNbins(), h2d->GetXaxis()->GetXmin(), h2d->GetXaxis()->GetXmax());
	TF1 *f1 = new TF1("f1", "gaus", 0, 1);
	TFitResultPtr fitResult;
	TMarker *m1;

	double xmin, xmax;

	std::ofstream ofile1("fitresults.dat");
	ofile1 << "id\tA\tmean\tsigma\tchi2\n";
	
	std::ofstream ofile2("time.cal");
	ofile2 << "#Set the time offset for a given scan channel (16*m + c, where m is the module\n";
	ofile2 << "# module and c is the channel) relative to the start detector. The following\n";
	ofile2 << "# operation is applied, T = T' - t0 where T is the calibrated time, T' is\n";
	ofile2 << "# the uncalibrated time, and t0 is given below (all in ns).\n";
	ofile2 << "#id	t0(ns)\n";

	for(int i = 1; i <= h2d->GetYaxis()->GetNbins(); i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(getProjectionX(h1, h2d, i)){ 
			std::cout << "DONE\n";

			std::stringstream stream; stream << h2d->GetYaxis()->GetBinLowEdge(i);
			h1->SetTitle(stream.str().c_str());
			
			can2->Clear();
			h1->Draw();
			can2->Update();
			
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			xmin = m1->GetX();
			m1->Delete();
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			xmax = m1->GetX();
			m1->Delete();
			
			std::cout << "  Range: " << xmin << ", " << xmax << std::endl;
			
			f1->SetRange(xmin, xmax);
			h1->Fit(f1, "QR");
			f1->Draw("SAME");
			
			// Output the fit results.
			std::cout << "  Fit: chi^2 = " << f1->GetChisquare()/f1->GetNDF() << ", t0 = " << f1->GetParameter(1) << " ns\n";
			ofile1 << stream.str() << "\t" << f1->GetParameter(0) << "\t" << f1->GetParameter(1) << "\t" << f1->GetParameter(2) << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;
			ofile2 << stream.str() << "\t" << f1->GetParameter(1) << std::endl;
			
			can2->WaitPrimitive();
		}
		else std::cout << "FAILED\n";
	}
	
	delete h1;
	delete f1;
	
	return true;
}

int main(int argc, char *argv[]){
	timeAlign obj;
	
	return obj.execute(argc, argv);
}
