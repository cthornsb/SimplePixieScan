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

bool GetProjectionX(TH1 *h1_, TH2 *h2_, const int &binY_){
	// Check that the histograms are defined.
	if(!h1_ || !h2_) return false;

	//std::cout << binY_ << "\t" << h2_->GetYaxis()->GetNbins() << "\t" << h1_->GetXaxis()->GetNbins() << "\t" << h2_->GetXaxis()->GetNbins() << std::endl;
	
	// Check for index out of range.
	if(binY_ < 0 || binY_ > h2_->GetYaxis()->GetNbins()) return false;
	
	// Check that both histograms have the same number of bins in the x-axis.
	if(h1_->GetXaxis()->GetNbins() != h2_->GetXaxis()->GetNbins()) return false;
	
	// Empty the output histogram.
	h1_->Reset();

	// Fill the output histogram.
	double total = 0.0;
	double binContent;
	for(int i = 0; i <= h1_->GetXaxis()->GetNbins()+1; i++){
		binContent = h2_->GetBinContent(h2_->GetBin(i, binY_));
		h1_->SetBinContent(i, binContent);
		total += binContent;
	}
	
	return (total > 0.0);
}

bool Process(TH2 *h_, TCanvas *can_){
	if(!h_ || !can_) return false;
	
	can_->cd()->SetLogy();
	
	TH1D *h1 = new TH1D("h1", "", h_->GetXaxis()->GetNbins(), h_->GetXaxis()->GetXmin(), h_->GetXaxis()->GetXmax());
	TF1 *f1 = new TF1("f1", "gaus", 0, 1);
	TFitResultPtr fitResult;
	TMarker *m1;

	double xmin, xmax;

	std::ofstream ofile("fitresults.dat");

	for(int i = 1; i <= h_->GetYaxis()->GetNbins(); i++){
		std::cout << " Processing channel ID " << i << "... ";
		if(GetProjectionX(h1, h_, i)){ 
			std::cout << "DONE\n";

			std::stringstream stream; stream << h_->GetYaxis()->GetBinLowEdge(i);
			h1->SetTitle(stream.str().c_str());
			
			can_->Clear();
			h1->Draw();
			can_->Update();
			
			m1 = (TMarker*)can_->WaitPrimitive("TMarker");
			xmin = m1->GetX();
			m1->Delete();
			m1 = (TMarker*)can_->WaitPrimitive("TMarker");
			xmax = m1->GetX();
			m1->Delete();
			
			std::cout << "  Range: " << xmin << ", " << xmax << std::endl;
			
			f1->SetRange(xmin, xmax);
			h1->Fit(f1, "QR");
			f1->Draw("SAME");
			
			// Output the fit results.
			std::cout << "  Fit: chi^2 = " << f1->GetChisquare()/f1->GetNDF() << ", t0 = " << f1->GetParameter(1) << " ns\n";
			ofile << stream.str() << "\t" << f1->GetParameter(0) << "\t" << f1->GetParameter(1) << "\t" << f1->GetParameter(2) << "\t" << f1->GetChisquare()/f1->GetNDF() << std::endl;
			
			can_->WaitPrimitive();
		}
		else std::cout << "FAILED\n";
	}
	
	delete h1;
	
	return true;
}

void help(char * prog_name_){
	std::cout << "  SYNTAX: " << prog_name_ << " <filename> <name> [options]\n";
	std::cout << "   Available options:\n";
	std::cout << "    --draw <drawstr>             | Draw a new histogram instead of loading one from the input file.\n";
	std::cout << "    --save <filename> <histname> | Save the 2d histogram to an output root file.\n";
}

int main(int argc, char *argv[]){
	if(argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)){
		help(argv[0]);
		return 1;
	}
	else if(argc < 3){
		std::cout << " Error: Invalid number of arguments to " << argv[0] << ". Expected 2, received " << argc-1 << ".\n";
		help(argv[0]);
		return 1;
	}

	std::string draw_string = "";
	std::string save_filename = "";
	std::string save_histname = "";
	bool create_hist = false;
	int index = 3;
	while(index < argc){
		if(strcmp(argv[index], "--draw") == 0){
			if(index + 1 >= argc){
				std::cout << " Error! Missing required argument to '--draw'!\n";
				help(argv[0]);
				return 1;
			}
			draw_string = std::string(argv[++index]);
			create_hist = true;
		}
		else if(strcmp(argv[index], "--save") == 0){
			if(index + 2 >= argc){
				std::cout << " Error! Missing required argument to '--save'!\n";
				help(argv[0]);
				return 1;
			}
			save_filename = std::string(argv[++index]);
			save_histname = std::string(argv[++index]);
		}
		else{ 
			std::cout << " Error! Unrecognized option '" << argv[index] << "'!\n";
			help(argv[0]);
			return 1;
		}
		index++;
	}

	TApplication* rootapp = new TApplication("rootapp", 0, NULL);
	
	TCanvas *can1 = new TCanvas("can1", "Canvas");
	can1->cd()->SetLogz();

	TH2 *hist = NULL;
	TFile *file = new TFile(argv[1], "READ");
	if(!file->IsOpen()){
		std::cout << " Error! Failed to open input file '" << argv[1] << "'.\n";
		return 1;
	}

	if(!create_hist){
		hist = (TH2*)file->Get(argv[2]);
		if(!hist){
			std::cout << " Error! Failed to load input histogram '" << argv[2] << "'.\n";
			file->Close();
			return 1;
		}
	}
	else{	
		TTree *tree = (TTree*)file->Get(argv[2]);
		if(!tree){
			std::cout << " Error! Failed to load input histogram '" << argv[2] << "'.\n";
			file->Close();
			return 1;
		}
	
		std::cout << " " << argv[2] << "->Draw(\"" << draw_string << "\", \"\", \"COLZ\");\n";
		std::cout << " Filling TH2... " << std::flush;	
		//tree->Draw("vandle.loc:vandle.ctof>>(250,-80,-20,42,16,58)"); // VANDLE
		//tree->Draw("liquidbar.loc:liquidbar.ctof>>(250,-80,-20,10,64,74)"); // NeuDLES
		//tree->Draw("generic.loc:generic.tof>>(250,-80,-20,10,80,90)"); // HAGRiD
		tree->Draw(draw_string.c_str(), "", "COLZ");
		std::cout << "DONE\n";
	
		hist = (TH2*)tree->GetHistogram();	
	}

	if(!save_filename.empty()){
		TFile *ofile = new TFile(save_filename.c_str(), "RECREATE");
		ofile->cd();
		hist->Write(save_histname.c_str());
		ofile->Close();
		std::cout << " Saved histogram '" << save_histname << "' to file '" << save_filename << "'.\n";
	}

	hist->Draw("COLZ");
	can1->WaitPrimitive();

	TCanvas *can2 = new TCanvas("can2", "Canvas");

	Process(hist, can2);

	file->Close();
	can1->Close();
	can2->Close();
	
	delete file;
	delete can1;
	delete can2;
	
	return 0;
}
