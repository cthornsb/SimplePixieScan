#include "OnlineProcessor.hpp"

#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TApplication.h"

/// Redraw all current plots to the canvas.
void OnlineProcessor::RedrawRoot(){
	can->Clear();
	can->Update();
}

OnlineProcessor::OnlineProcessor(){
	init = false;
}

OnlineProcessor::~OnlineProcessor(){
	if(init){
		can->Close();
		delete can;
		for(unsigned int i = 0; i < 5; i++){
			delete hists_1d[i];
		}
	}
}

/// Initialize.
bool OnlineProcessor::Initialize(){
	if(init){ return false; }

	// Variables for root graphics
	rootapp = new TApplication("rootapp", 0, NULL);
	gSystem->Load("libTree");

	// Setup the root canvas for plotting.
	can = new TCanvas("scanner_c1", "Scanner Canvas");
	can->Divide(2, 2);

	// Setup all 1d online diagnostic histograms.
	hists_1d[0] = new TH1D("1h1", "", 1, 0, 1); 
	hists_1d[1] = new TH1D("1h2", "", 1, 0, 1);
	hists_1d[2] = new TH1D("1h3", "", 1, 0, 1);
	hists_1d[3] = new TH1D("1h4", "", 1, 0, 1);
	hists_1d[4] = new TH1D("1h5", "", 1, 0, 1);
	
	// Setup all 1d online diagnostic histograms.
	hists_2d[0] = new TH2D("2h1", "", 1, 0, 1, 1, 0, 1); 
	hists_2d[1] = new TH2D("2h2", "", 1, 0, 1, 1, 0, 1);
	hists_2d[2] = new TH2D("2h3", "", 1, 0, 1, 1, 0, 1);
	hists_2d[3] = new TH2D("2h4", "", 1, 0, 1, 1, 0, 1);
	hists_2d[4] = new TH2D("2h5", "", 1, 0, 1, 1, 0, 1);
	
	return (init = true);
}

/// Set the range of a 1d histogram.
void OnlineProcessor::SetRange(const unsigned int &index_, const double &low_, const double &high_){
}

/// Set the range of a 2d histogram.
void OnlineProcessor::SetRange(const unsigned int &index_, const double &Xlow_, const double &Xhigh_, const double &Ylow_, const double &Yhigh_){
}

/// Refresh online plots.
void OnlineProcessor::Refresh(){
}
