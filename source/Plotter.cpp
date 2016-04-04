#include <sstream>

#include "Plotter.hpp"

#include "TH1.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TPad.h"

Plotter::Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_,
                 const std::string &xtitle_, const int &xbins_, const double &xmin_, const double &xmax_){
	name = name_;
	opt = draw_opt_;
	xmin = xmin_;
	xmax = xmax_;
	hist = (TH1*)(new TH1F(name.c_str(), title_.c_str(), xbins_, xmin, xmax));
	hist->GetXaxis()->SetTitle(xtitle_.c_str());
	std::stringstream stream; stream << "Counts per " << (xmax_-xmin_)/xbins_ << " units";
	hist->GetYaxis()->SetTitle(stream.str().c_str());
	hist->SetStats(0);
	logx = false;
	logy = false;
	logz = false;
	dim = 1;
}

Plotter::Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_,
                 const std::string &xtitle_, const int &xbins_, const double &xmin_, const double &xmax_,
                 const std::string &ytitle_, const int &ybins_, const double &ymin_, const double &ymax_){
	name = name_;
	opt = draw_opt_;
	xmin = xmin_;
	xmax = xmax_;
	ymin = ymin_;
	ymax = ymax_;
	hist = (TH1*)(new TH2F(name.c_str(), title_.c_str(), xbins_, xmin, xmax, ybins_, ymin, ymax));
	hist->GetXaxis()->SetTitle(xtitle_.c_str());
	hist->GetYaxis()->SetTitle(ytitle_.c_str());
	hist->SetStats(0);
	logx = false;
	logy = false;
	logz = false;
	dim = 2;
}

Plotter::~Plotter(){ 
	delete hist; 
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

void Plotter::SetXaxisTitle(const std::string &title_){ 
	hist->GetXaxis()->SetTitle(title_.c_str()); 
}

void Plotter::SetYaxisTitle(const std::string &title_){ 
	hist->GetYaxis()->SetTitle(title_.c_str()); 
}

void Plotter::SetStats(const bool &state_/*=true*/){
	hist->SetStats(state_); 
}

void Plotter::SetXrange(const double &xmin_, const double &xmax_){
	xmin = xmin_;
	xmax = xmax_;
	hist->GetXaxis()->SetRangeUser(xmin, xmax);
}

void Plotter::SetYrange(const double &ymin_, const double &ymax_){
	ymin = ymin_;
	ymax = ymax_;
	hist->GetYaxis()->SetRangeUser(ymin, ymax);
}

void Plotter::SetRange(const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_){
	SetXrange(xmin_, xmax_);
	SetYrange(ymin_, ymax_);
	hist->GetXaxis()->SetRangeUser(xmin, xmax);
	hist->GetYaxis()->SetRangeUser(ymin, ymax);
}

void Plotter::ResetXrange(){
	hist->GetXaxis()->UnZoom();
}

void Plotter::ResetYrange(){
	hist->GetYaxis()->UnZoom();
}

void Plotter::ResetRange(){
	hist->GetXaxis()->UnZoom();
	hist->GetYaxis()->UnZoom();
}

void Plotter::Zero(){
	hist->Reset();
}

void Plotter::Fill(const double &x_, const double &y_/*=0.0*/){
	if(dim == 1){ hist->Fill(x_); }
	else{ hist->Fill(x_, y_); }
}

void Plotter::Draw(TPad *pad_){
	/*if(dim == 1){
		SetYrange(hist->GetMinimum()*1.1, hist->GetMaximum()*1.1);
	}*/
	if(logx){ pad_->SetLogx(); }
	if(logy){ pad_->SetLogy(); }
	if(logz){ pad_->SetLogz(); }
	hist->Draw((opt+"SAME").c_str());
}

void Plotter::Write(){
	hist->Write();
}
