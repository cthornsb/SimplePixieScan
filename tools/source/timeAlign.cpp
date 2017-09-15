#include <iostream>
#include <sstream>
#include <fstream>

#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TLine.h"
#include "TH1.h"
#include "TH2.h"
#include "TF1.h"
#include "TH1D.h"
#include "TMarker.h"

#include "simpleTool.hpp"


/** 1d double Woods-Saxon potential.
  * \param[in] x: x[0] = dT = tR-tL in ns
  * \param[in] p:
  *  p[0] = Amplitude at T0
  *  p[1] = beta (FWHM) in ns
  *  p[2] = T0 (time offset) in ns
  *  p[3] = a (slope of left side)
  *  p[4] = b (slope of right side)
  */
double doubleWoodsSaxon(double *x, double *p){
	if(x[0] < p[2]){ // Left side
		return ((-p[0]/(1+std::exp((x[0]-0.5*p[1]-p[2])/p[3])))+p[0]);
	}
	// Right side
	return (p[0]/(1+std::exp((x[0]+0.5*p[1]-p[2])/p[4])));
}

class timeAlign : public simpleHistoFitter {
  private:
	double fitRangeMult; // Range of fit in multiples of beta (FWHM) of distribution.
	double detectorLength; // Total lenght of detector (in cm).

  public:
	timeAlign() : simpleHistoFitter(), fitRangeMult(1.5), detectorLength(60) { }

	void addChildOptions();

	bool processChildArgs(); 

	bool process();
};

void timeAlign::addChildOptions(){
	addOption(optionExt("fit-range", required_argument, NULL, 0, "<multiplier>", "Specify FWHM multiplier for range of fit of distribution (default = 1.5)."), userOpts, optstr);
	addOption(optionExt("length", required_argument, NULL, 0, "<length>", "Specify the length of the detectors (in cm, default = 60)."), userOpts, optstr);
}

bool timeAlign::processChildArgs(){
	if(userOpts.at(firstChildOption).active)
		fitRangeMult = strtod(userOpts.at(firstChildOption).argument.c_str(), NULL);
	if(userOpts.at(firstChildOption+1).active)
		detectorLength = strtod(userOpts.at(firstChildOption+1).argument.c_str(), NULL);

	return true;
}

bool timeAlign::process(){
	if(!h2d || !can2) return false;
	
	can2->cd();//->SetLogy();

	std::cout << "Using detector length of " << detectorLength << " cm.\n";
	std::cout << "Using fitting range of " << fitRangeMult << " multiples of beta.\n"; 
	
	TH1D *h1 = getProjectionHist(h2d);
	TF1 *f1 = new TF1("f1", doubleWoodsSaxon, 0, 1, 5);
	TMarker *m1;

	double xmin, xmax;

	std::ofstream ofile1("fitresults.dat");
	ofile1 << "id\tv0\tbeta\tt0\ta\tb\tchi2\n";
	
	std::ofstream ofile2("bars.cal");
	ofile2 << "#Set the time offset for a given scan channel (16*m + c, where m is the module\n";
	ofile2 << "# module and c is the channel) relative to the start detector. The following\n";
	ofile2 << "# operation is applied, T = T' - t0 where T is the calibrated time, T' is\n";
	ofile2 << "# the uncalibrated time, and t0 is given below (all in ns).\n";
	ofile2 << "#id\tt0(ns)\tcbar(cm/ns)\n";

	// Initial fitting parameters.
	double v0, beta, t0, a, b;
	double crossLeft, crossRight;
	double cbar;

	int numProjections = getNumProjections(h2d);
	for(int i = 1; i <= numProjections; i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(getProjection(h1, h2d, i)){ 
			std::cout << "DONE\n";

			std::stringstream stream; stream << getBinLowEdge(h2d, i);
			h1->SetTitle(stream.str().c_str());
			
			can2->Clear();
			h1->Draw();
			can2->Update();
		
			// Get the amplitude of the distribution from the user.
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			t0 = m1->GetX();
			v0 = m1->GetY();
			delete m1;	
			
			// Draw the FWHM line.
			TLine line(h1->GetXaxis()->GetXmin(), v0/2, h1->GetXaxis()->GetXmax(), v0/2);
			line.Draw("SAME");
			can2->Update();

			// Get the left crossing point.
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			crossLeft = m1->GetX();
			delete m1;	
			
			// Get the left crossing point.
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			crossRight = m1->GetX();
			delete m1;

			// Calculate the FWHM.
			beta = crossRight-crossLeft;	

			a = 0.5; // ?
			b = 0.5; // ?

			/*m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			xmin = m1->GetX();
			m1->Delete();
			m1 = (TMarker*)can2->WaitPrimitive("TMarker");
			xmax = m1->GetX();
			m1->Delete();*/
			
			xmin = t0-fitRangeMult*(beta/2);
			xmax = t0+fitRangeMult*(beta/2);

			std::cout << "  Range: " << xmin << ", " << xmax << std::endl;
	
			f1->SetParameters(v0, beta, t0, a, b);	
			f1->SetRange(xmin, xmax);
			h1->Fit(f1, "QR");
			f1->Draw("SAME");
			
			// Output the fit results.
			/*std::cout << "  Fit: chi^2 = " << f1->GetChisquare()/f1->GetNDF() << ", t0 = " << f1->GetParameter(1) << " ns\n";
			ofile1 << stream.str() << "\t" << f1->GetParameter(0) << "\t" << f1->GetParameter(1) << "\t" << f1->GetParameter(2) << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;
			ofile2 << stream.str() << "\t" << f1->GetParameter(1) << std::endl;*/

			// Calculate bar speed-of-light.
			cbar = 2*detectorLength/beta;

			std::cout << "  Fit: chi^2 = " << f1->GetChisquare()/f1->GetNDF() << ", t0 = " << f1->GetParameter(1) << " ns, cbar  = " << cbar << " cm/ns\n";
			ofile1 << stream.str();
			for(int j = 0; j < 5; j++) ofile1 << "\t" << f1->GetParameter(j);
			ofile1 << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;
			ofile2 << stream.str() << "\t" << f1->GetParameter(2) << "\t" << cbar << std::endl;
			
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
