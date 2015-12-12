#include <iostream>

#include "Processor.hpp"
#include "OnlineProcessor.hpp"

#include "TH1.h"
#include "TCanvas.h"
#include "TSystem.h"
#include "TApplication.h"

plot_object::plot_object(TH1 *hist_, const std::string &draw_opt_/*=""*/){
	hist = hist_;
	opt = draw_opt_;
	name = std::string(hist->GetName());
}

void plot_object::Draw(){
	hist->Draw(opt.c_str());
}

OnlineProcessor::OnlineProcessor(const unsigned int &cols_/*=2*/, const unsigned int &rows_/*=2*/){
	num_hists = cols_*rows_;
	canvas_cols = cols_;
	canvas_rows = rows_;
	
	// Setup arrays.
	which_hists = new int[cols_*rows_];
	
	// Set the histogram ids for all TPads.
	for(unsigned int i = 0; i < num_hists; i++){
		which_hists[i] = -1;
	}
	
	// Variables for root graphics
	rootapp = new TApplication("rootapp", 0, NULL);
	gSystem->Load("libTree");

	// Setup the root canvas for plotting.
	can = new TCanvas("scanner_c1", "Scanner Canvas");
}

OnlineProcessor::~OnlineProcessor(){
	can->Close();
	delete can;
	delete[] which_hists;
}

bool OnlineProcessor::ChangeHist(const unsigned int &index_, const size_t &hist_id_){
	if(index_ >= num_hists || hist_id_ >= plottable_hists.size()){ return false; }
	which_hists[index_] = hist_id_;
	return true;
}

bool OnlineProcessor::ChangeHist(const unsigned int &index_, const std::string &hist_name_){
	if(index_ >= num_hists){ return false; }
	
	int count = 0;
	for(std::vector<TH1*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
		if(strcmp((*iter)->GetName(), hist_name_.c_str()) == 0){
			which_hists[index_] = count;
			return true;
		}
		count++;
	}
	
	return false;
}

/// Refresh online plots.
void OnlineProcessor::Refresh(){
	can->Clear();

	can->Divide(canvas_cols, canvas_rows);

	std::vector<TH1*>::iterator iter = plottable_hists.begin();

	// Set the histogram ids for all TPads.
	for(unsigned int i = 0; i < num_hists; i++){
		can->cd(i+1);
		if(which_hists[i] >= 0){
			(*(iter+which_hists[i]))->Draw();
		}
	}

	can->Update();
}

/// Add a processor's histograms to the list of plottable items.
void OnlineProcessor::AddHists(Processor *proc){
	std::vector<TH1*> processor_hists;
	proc->GetHists(processor_hists);
	for(std::vector<TH1*>::iterator iter = processor_hists.begin(); iter != processor_hists.end(); iter++){
		plottable_hists.push_back((*iter));
	}
	processor_hists.clear();
}

/// Add a single histogram to the list of plottable items.
void OnlineProcessor::AddHist(TH1 *hist_){
	plottable_hists.push_back(hist_);
}

/// Display a list of available plots.
void OnlineProcessor::PrintHists(){
	std::cout << "OnlineProcessor: Displaying list of plottable histograms.\n";
	
	if(plottable_hists.empty()){
		std::cout << " NONE\n";
		return;
	}
	
	int count = 0;
	for(std::vector<TH1*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
		std::cout << " " << count++ << ": " << (*iter)->GetName() << "\t" << (*iter)->GetTitle() << std::endl;
	}
}
