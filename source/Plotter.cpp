#include <iostream>
#include <sstream>

#include "Plotter.hpp"

#include "TH1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TPad.h"
#include "TFile.h"

Plotter::Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_, 
                 const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_) :
                 dim(1), xbins(xbins_), ybins(0), 
                 xmin(xmin_), xmax(xmax_), 
                 ymin(0), ymax(0),
                 logx(false), logy(false), logz(false),
                 name(name_), title(title_), opt(draw_opt_), 
                 xtitle(xtitle_), ytitle(), 
                 xunits(xunits_), yunits(),
                 numHists(0), hist(NULL)
{
	hist = (TH1*)(new TH1F(name.c_str(), title.c_str(), xbins, xmin, xmax));
	SetupHist1d(hist);
}

Plotter::Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_,
	             const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_, const std::string &ytitle_,
                 const std::string &yunits_, const int &ybins_, const double &ymin_, const double &ymax_) :
                 dim(2), xbins(xbins_), ybins(ybins_), 
                 xmin(xmin_), xmax(xmax_), 
                 ymin(ymin_), ymax(ymax_),
                 logx(false), logy(false), logz(false),
                 name(name_), title(title_), opt(draw_opt_), 
                 xtitle(xtitle_), ytitle(ytitle_), 
                 xunits(xunits_), yunits(yunits_),
                 numHists(0), hist(NULL)
{
	hist = (TH1*)(new TH2F(name.c_str(), title.c_str(), xbins, xmin, xmax, ybins, ymin, ymax));
	SetupHist2d(hist);
}

Plotter::~Plotter(){ 
	delete hist;
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		delete iter->first;
}

TH1 *Plotter::AddNew1dHistogram(const int &location, const std::string &newTitle/*=""*/){
	numHists++;
	std::stringstream newName;
	newName << name << "-" << location;
	hists1d.push_back(std::pair<TH1*, int>((TH1*)(new TH1F(newName.str().c_str(), (newTitle.empty() ? title.c_str() : newTitle.c_str()), xbins, xmin, xmax)), location));
	SetupHist1d(hists1d.back().first);
	return hists1d.back().first;
}

TH1 *Plotter::AddNew2dHistogram(const int &location, const std::string &newTitle/*=""*/){
	numHists++;
	std::stringstream newName;
	newName << name << "-" << location;
	hists1d.push_back(std::pair<TH1*, int>((TH1*)(new TH2F(newName.str().c_str(), (newTitle.empty() ? title.c_str() : newTitle.c_str()), xbins, xmin, xmax, ybins, ymin, ymax)), location));
	SetupHist2d(hists1d.back().first);
	return hists1d.back().first;	
}

TH1 *Plotter::GetHist(const int &location){
	std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin();
	for(; iter != hists1d.end(); iter++)
		if(iter->second == location) return iter->first;
	return NULL;
}

void Plotter::GetXrange(double &xmin_, double &xmax_){
	xmin_ = xmin;
	xmax_ = xmax;
}

void Plotter::GetYrange(double &ymin_, double &ymax_){
	ymin_ = ymin;
	ymax_ = ymax;
}

void Plotter::GetRange(double &xmin_, double &xmax_, double &ymin_, double &ymax_){
	xmin_ = xmin;
	xmax_ = xmax;
	ymin_ = ymin;
	ymax_ = ymax;
}

bool Plotter::DetectorIsDefined(const int &id) const {
	for(std::vector<std::pair<TH1*, int> >::const_iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		if(iter->second == id) return true;	
	return false;
}

void Plotter::SetXaxisTitle(const std::string &title_){ 
	hist->GetXaxis()->SetTitle(title_.c_str()); 
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetXaxis()->SetTitle(title_.c_str()); 
}

void Plotter::SetYaxisTitle(const std::string &title_){ 
	hist->GetYaxis()->SetTitle(title_.c_str()); 
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetYaxis()->SetTitle(title_.c_str()); 
}

void Plotter::SetStats(const bool &state_/*=true*/){
	hist->SetStats(state_);
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->SetStats(state_); 
}

void Plotter::SetXrange(const double &xmin_, const double &xmax_){
	xmin = xmin_;
	xmax = xmax_;
	hist->GetXaxis()->SetRangeUser(xmin, xmax);
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetXaxis()->SetRangeUser(xmin, xmax);
}

void Plotter::SetYrange(const double &ymin_, const double &ymax_){
	ymin = ymin_;
	ymax = ymax_;
	hist->GetYaxis()->SetRangeUser(ymin, ymax);
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetYaxis()->SetRangeUser(ymin, ymax);
}

void Plotter::SetRange(const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_){
	SetXrange(xmin_, xmax_);
	SetYrange(ymin_, ymax_);
}

void Plotter::SetupHist1d(TH1 *h){
	if(!xunits.empty())
		h->GetXaxis()->SetTitle((xtitle+" ("+xunits+")").c_str());
	else
		h->GetXaxis()->SetTitle(xtitle.c_str());
	std::stringstream stream; stream << "Counts per " << (xmax-xmin)/xbins << " " << (xunits.empty() ? " units" : xunits);
	h->GetYaxis()->SetTitle(stream.str().c_str());
	h->SetStats(0);
}

void Plotter::SetupHist2d(TH1 *h){
	if(!xunits.empty())
		h->GetXaxis()->SetTitle((xtitle+" ("+xunits+")").c_str());
	else
		h->GetXaxis()->SetTitle(xtitle.c_str());
	if(!yunits.empty())
		h->GetYaxis()->SetTitle((ytitle+" ("+yunits+")").c_str());
	else
		h->GetYaxis()->SetTitle(ytitle.c_str());
	h->SetStats(0);
}

void Plotter::ResetXrange(){
	hist->GetXaxis()->UnZoom();
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetXaxis()->UnZoom();
}

void Plotter::ResetYrange(){
	hist->GetYaxis()->UnZoom();
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->GetYaxis()->UnZoom();
}

void Plotter::ResetRange(){
	ResetXrange();
	ResetYrange();
}

void Plotter::Zero(){
	hist->Reset();
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->Reset();
}

void Plotter::Fill(const double &x_){
	hist->Fill(x_);
}

void Plotter::Fill(const int &detID, const double &x_){
	hist->Fill(x_, detID); 
	for(std::vector<std::pair<TH1*, int> >::const_iterator iter = hists1d.begin(); iter != hists1d.end(); iter++){
		if(iter->second == detID)
			iter->first->Fill(x_);
	}	
}

void Plotter::Fill2d(const double &x_, const double &y_){
	hist->Fill(x_, y_); 
}

void Plotter::Fill2d(const int &detID, const double &x_, const double &y_){
	hist->Fill(x_, y_);
	for(std::vector<std::pair<TH1*, int> >::const_iterator iter = hists1d.begin(); iter != hists1d.end(); iter++){
		if(iter->second == detID)
			iter->first->Fill(x_, y_);
	}
}

void Plotter::Draw(TPad *pad_, const int &detID/*=-1*/){
	if(logx){ pad_->SetLogx(); }
	if(logy){ pad_->SetLogy(); }
	if(logz){ pad_->SetLogz(); }
	if(detID < 0)
		hist->Draw(opt.c_str());
	else{
		for(std::vector<std::pair<TH1*, int> >::const_iterator iter = hists1d.begin(); iter != hists1d.end(); iter++){
			if(iter->second == detID){
				if(dim == 1)
					iter->first->Draw();
				else
					iter->first->Draw(opt.c_str());
				break;
			}
		}
	}
}

void Plotter::Write(TFile *file_, const std::string &dirname_/*="hists"*/){
	if(hists1d.empty()){
		if(!file_->cd(dirname_.c_str())) 
			return;
	}
	else{
		file_->mkdir((dirname_+"/"+name).c_str());
		file_->cd((dirname_+"/"+name).c_str());	
	}
	hist->Write();
	for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
		iter->first->Write();
}

void Plotter::Print(){
	std::cout << name << "\t" << (dim == 1 ? "1d" : "2d") << "\t" << title;
	if(!hists1d.empty()){
		std::cout << " [ ";
		for(std::vector<std::pair<TH1*, int> >::iterator iter = hists1d.begin(); iter != hists1d.end(); iter++)
			std::cout << iter->second << " ";
		std::cout << "]";
	}
	std::cout << std::endl;
}
