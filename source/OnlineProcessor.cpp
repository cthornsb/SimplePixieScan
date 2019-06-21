#include <iostream>

#include "ColorTerm.hpp"
#include "CTerminal.h"
#include "Processor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"
#include "OnlineProcessor.hpp"

#include "TH1.h"
#include "TFile.h"
#include "TCanvas.h"

///////////////////////////////////////////////////////////////////////////////
// class HistoDefFile
///////////////////////////////////////////////////////////////////////////////

HistoDefFile::HistoDefFile(const char *fname){
	ReadMap(fname);
}

bool HistoDefFile::ReadMap(const char *fname){
	std::ifstream ifile(fname);
	if(!ifile.good()) return false;

	std::string currType = "";
	std::string line;
	while(true){
		getline(ifile, line);
		if(ifile.eof()) break;
		if(line.empty() || line[0] == '#') continue;
	
		std::string type = line.substr(0, line.find_first_of(':'));
		if(type != currType){
			currType = type;
			histMap[type] = std::deque<std::string>();
		}
		histMap.at(currType).push_back(line);
	}

	ifile.close();
	
	return (!histMap.empty());
}

bool HistoDefFile::GetNext(const std::string &type, std::string &line){
	for(std::map<std::string, std::deque<std::string> >::iterator iter = histMap.begin(); iter != histMap.end(); iter++){
		if(iter->first == type){
			if(iter->second.empty()) return false;
			line = iter->second.front();
			iter->second.pop_front();
			return true;
		}
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// class OnlineProcessor
///////////////////////////////////////////////////////////////////////////////

TPad *OnlineProcessor::cd(const unsigned int &index_){
	if(!GetPlot(index_)){ return NULL; }
	pad = (TPad*)(can->cd(index_+1));
	plot = plottable_hists.at(which_hists[index_].first);
	return pad;
}

OnlineProcessor::OnlineProcessor(){
	display_mode = false;
	hadHistError = false;
}

OnlineProcessor::~OnlineProcessor(){
	if(display_mode){
		can->Close();
		delete can;
	}

	// Delete all defined histograms.
	for(std::vector<Plotter*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
	}
}

Plotter* OnlineProcessor::GetPlot(const unsigned int &index_){
	if(!display_mode || index_ >= num_hists){ return NULL; }
	return (plot = plottable_hists[index_]);
}

bool OnlineProcessor::ReadHistMap(const char *fname){
	return histMap.ReadMap(fname);
}

bool OnlineProcessor::SetDisplayMode(const unsigned int &cols_/*=2*/, const unsigned int &rows_/*=2*/){
	if(display_mode){ return false; }

	num_hists = cols_*rows_;
	canvas_cols = cols_;
	canvas_rows = rows_;
	
	// Setup arrays.
	which_hists = std::vector<std::pair<int, int> >(cols_*rows_, std::pair<int, int>(-1, -1));
	
	// Setup the root canvas for plotting.
	can = new TCanvas("scanner_c1", "Scanner Canvas");
	
	return (display_mode=true);
}

bool OnlineProcessor::ChangeHist(const unsigned int &index_, const unsigned int &hist_id_, const int &det_id_/*=-1*/){
	if(!display_mode || index_ >= num_hists || hist_id_ >= plottable_hists.size()){ return false; }
	which_hists[index_].first = hist_id_;
	if(det_id_ >= 0){
		if(!plottable_hists[hist_id_]->DetectorIsDefined(det_id_)){
			which_hists[index_].second = -1;
			return false;
		}
		which_hists[index_].second = det_id_;
	}
	else
		which_hists[index_].second = -1;
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ChangeHist(const unsigned int &index_, const std::string &hist_name_){
	if(!display_mode || index_ >= num_hists){ return false; }
	
	int count = 0;
	for(std::vector<Plotter*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
		if(strcmp((*iter)->GetName().c_str(), hist_name_.c_str()) == 0){
			which_hists[index_].first = count;
			which_hists[index_].second  = -1;
			Refresh(index_);
			return true;
		}
		count++;
	}
	
	return false;
}

bool OnlineProcessor::SetXrange(const unsigned int &index_, const double &xmin_, const double &xmax_){
	if(!GetPlot(index_)){ return false; }
	plot->SetXrange(xmin_, xmax_);
	Refresh(index_);
	return true;
}

bool OnlineProcessor::SetYrange(const unsigned int &index_, const double &ymin_, const double &ymax_){
	if(!GetPlot(index_)){ return false; }
	plot->SetYrange(ymin_, ymax_);
	Refresh(index_);
	return true;
}

bool OnlineProcessor::SetRange(const unsigned int &index_, const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_){
	if(!GetPlot(index_)){ return false; }
	plot->SetXrange(xmin_, xmax_);
	plot->SetYrange(ymin_, ymax_);
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ResetXrange(const unsigned int &index_){
	if(!GetPlot(index_)){ return false; }
	plot->GetHist()->GetXaxis()->UnZoom();
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ResetYrange(const unsigned int &index_){
	if(!GetPlot(index_)){ return false; }
	plot->GetHist()->GetYaxis()->UnZoom();
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ResetRange(const unsigned int &index_){
	if(!GetPlot(index_)){ return false; }
	plot->GetHist()->GetXaxis()->UnZoom();
	plot->GetHist()->GetYaxis()->UnZoom();
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ToggleLogX(const unsigned int &index_){
	if(!cd(index_) || plot->GetXmin() <= 0.0){ return false; }
	plot->ToggleLogX();
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ToggleLogY(const unsigned int &index_){
	if(!cd(index_) || plot->GetYmin() <= 0.0){ return false; }
	plot->ToggleLogY();
	Refresh(index_);
	return true;
}

bool OnlineProcessor::ToggleLogZ(const unsigned int &index_){
	if(!cd(index_)){ return false; }
	plot->ToggleLogZ();
	Refresh(index_);
	return true;
}

void OnlineProcessor::Refresh(const unsigned int &index_){
	if(cd(index_)){
		plot->Draw(pad, which_hists[index_].second);
		can->Update();
	}
}

void OnlineProcessor::Refresh(){
	if(!display_mode){ return; }

	can->Clear();

	// Divide the canvas into TPads.
	can->Divide(canvas_cols, canvas_rows);

	// Set the histogram ids for all TPads.
	for(unsigned int i = 0; i < num_hists; i++){
		if(which_hists[i].first >= 0){
			cd(i);
			plot->Draw(pad, which_hists[i].second);
		}
	}

	can->Update();
}

bool OnlineProcessor::Zero(const unsigned int &hist_id_){
	if(hist_id_ >= plottable_hists.size()){ return false; }
	plottable_hists.at(hist_id_)->Zero();
	return true;
}

void OnlineProcessor::AddHist(Plotter *hist_){
	plottable_hists.push_back(hist_);
}

void OnlineProcessor::StartAddHists(Processor *proc){
	hadHistError = false;

	// Get the first and last occurance of this type in the map.
	type = proc->GetType();
	name = proc->GetName();

	// Get all detector IDs from the map file
	locations.clear();
	mapfile->GetAllOccurances(type, locations, proc->GetIsSingleEnded());
	
	minloc = locations.front();
	maxloc = locations.back();

	histID = 1;

	// Get pointers to all histograms.
	proc->GetHists(this);
}

bool OnlineProcessor::GenerateHist(Plotter* &hist_){
	// Find this histogram in the map of histograms (hist.dat).
	std::string histString;
	if(!histMap.GetNext(type, histString)){
		warnStr << "OnlineProcessor: ERROR! Failed to find histogram definition for histogram type=" << type << "!\n";
		hadHistError = true;
		return false;
	}

	std::vector<std::string> args;
	unsigned int retval = split_str(histString, args, ':');
	if(retval < 7){
		warnStr << "OnlineProcessor: ERROR! Invalid number of arguments given to histogram builder (" << retval << "!\n";
		hadHistError = true;
		return false;
	}

	std::stringstream stream;
	stream << type << "_h" << histID++;
	if(retval < 12){ // Define a 1d histogram
		int bins = strtoul(args.at(4).c_str(), NULL, 10);
		double xlow = strtod(args.at(5).c_str(), NULL);
		double xhigh = strtod(args.at(6).c_str(), NULL);
		size_t nDet = maxloc-minloc+1;

		if(nDet >= 2){ // More than one detector. Define a 2d plot and a bunch of 1d plots
			std::stringstream stream2; stream2 << name << " Location vs. " << args.at(2);
			hist_ = new Plotter(stream.str(), stream2.str(), args.at(1), args.at(2), args.at(3), bins, xlow, xhigh, "Location", "", nDet, minloc, maxloc+1);
			for(std::vector<int>::iterator iter = locations.begin(); iter != locations.end(); iter++){
				hist_->AddNew1dHistogram((*iter));
			}
		}
		else{ // Only one detector. Define a 1d plot
			std::stringstream stream2; stream2 << name << " " << args.at(2);
			hist_ = new Plotter(stream.str(), stream2.str(), args.at(1), args.at(2), args.at(3), bins, xlow, xhigh);
		}
		plottable_hists.push_back(hist_);
	}
	else{ // Define a 2d histogram
		int xbins = strtoul(args.at(4).c_str(), NULL, 10);
		double xlow = strtod(args.at(5).c_str(), NULL);
		double xhigh = strtod(args.at(6).c_str(), NULL);

		int ybins = strtoul(args.at(9).c_str(), NULL, 10);
		double ylow = strtod(args.at(10).c_str(), NULL);
		double yhigh = strtod(args.at(11).c_str(), NULL);
		size_t nDet = maxloc-minloc+1;
		
		std::stringstream stream2; stream2 << name << " " << args.at(2) << " vs. " << args.at(7);
		hist_ = new Plotter(stream.str(), stream2.str(), args.at(1), args.at(2), args.at(3), xbins, xlow, xhigh, args.at(7), args.at(8), ybins, ylow, yhigh);
		
		if(nDet >= 2){ // More than one detector. Define a bunch of 2d plots
			for(std::vector<int>::iterator iter = locations.begin(); iter != locations.end(); iter++)
				hist_->AddNew2dHistogram((*iter));
		}
		else{ // Only one detector. Define a single 2d plot
		}
		plottable_hists.push_back(hist_);
	}

	return true;
}

void OnlineProcessor::GenerateLocationHist(Plotter* &hist_){
	// Define a generic location histogram.
	hist_ = new Plotter(type+"_loc", "GenericBar Location", "", "Location", "", (maxloc+1)-minloc, minloc, maxloc+1);

	plottable_hists.push_back(hist_);
}

int OnlineProcessor::WriteHists(TFile *file_, const std::string &dirname_/*="hists"*/){
	if(!file_){ return -1; }

	file_->mkdir(dirname_.c_str());

	int count = 0;
	for(std::vector<Plotter*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
		(*iter)->Write(file_, dirname_);
		count++;
	}
	
	return count;
}

void OnlineProcessor::PrintHists(){
	std::cout << "OnlineProcessor: Displaying list of plottable histograms.\n";
	
	if(plottable_hists.empty()){
		std::cout << " NONE\n";
		return;
	}
	
	int count = 0;
	for(std::vector<Plotter*>::iterator iter = plottable_hists.begin(); iter != plottable_hists.end(); iter++){
		std::cout << " " << count++ << ": "; 
		(*iter)->Print();
	}
}
