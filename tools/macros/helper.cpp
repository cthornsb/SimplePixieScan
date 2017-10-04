// helper.cpp
// Author: Cory R. Thornsberry
// Updated: May 18th, 2017

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const double Mn = 10454.0750977429; // Mass of neutron, in MeV.
const double pi = 3.1415926536; // pi constant.
const double twopi = 6.2831853072; // 2*pi constant.

// Array of root colors.
const short colors[6] = {kBlue+2, kRed, kGreen+2, kOrange+7, kMagenta+2, kCyan+2};

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

// Convert logic signal period to beam current (enA).
void conv(double &x, double &y){
	x *= 8E-9;
	y = 1E8/(8*y);
}

// Determine the mean beam current from a logic signal TGraph.
double mean(TGraph *g){
	double sum = 0.0;
	double x, y;
	for(int i = 0; i < g->GetN(); i++){
		g->GetPoint(i, x, y);
		conv(x, y);
		sum += y;
	}
	return sum/g->GetN();
}

// Convert a logic signal TGraph to a beam current vs. time graph.
TGraph *convert(TGraph *g, const char *name){
	TGraph *output = (TGraph*)(g->Clone(name));

	double x, y;
	for(int i = 0; i < g->GetN(); i++){
		g->GetPoint(i, x, y);
		conv(x, y);
		output->SetPoint(i, x, y);
	}

	return output;
}

// Return the range of bins containing an upper and lower point, rounded to the nearest bins.
void findBins(TH1 *h, const double &xstart_, const double &xstop_, int &lowBin, int &highBin){
	if(xstart_ <= h->GetXaxis()->GetXmin()) lowBin = 1;
	else                                    lowBin = h->FindBin(xstart_);
	if(xstop_ >= h->GetXaxis()->GetXmax())  highBin = h->GetNbinsX();
	else                                    highBin = h->FindBin(xstop_);
}

// Return the total integral of a logic signal TGraph.
double integrate(TGraph *g){
	double sum = 0.0;
	double integral = 0.0;
	double x1, y1;
	double x2, y2;
	g->GetPoint(0, x1, y1);
	conv(x1, y1);
	sum += y1;
	for(int i = 1; i < g->GetN(); i++){
		g->GetPoint(i, x2, y2);
		conv(x2, y2);
		sum += y2;
		integral += 0.5 * (y1 + y2) * (x2 - x1);
		x1 = x2;
		y1 = y2;
	}
	std::cout << "mean = " << sum/g->GetN() << " enA, integral = " << integral << " nC\n";
	return integral;
}

// Return the total integral of a 1-d histogram.
//  NOTE: This function returns the integral of the histogram, not the number of counts.
double integrate(TH1 *h){
	double sum = 0;
	for(int i = 1; i < h->GetNbinsX(); i++){
		sum += 0.5 * (h->GetBinContent(i) + h->GetBinContent(i+1)) * h->GetBinWidth(i);
	}
	return sum;
}

// Return the integral of a 1-d histogram in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the integral of the histogram, not the number of counts.
double integrate(TH1 *h, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	for(int i = lowBin; i < highBin; i++)
		retval += 0.5 * (h->GetBinContent(i) + h->GetBinContent(i+1)) * h->GetBinWidth(i);
	return retval;
}

// Integrate a root function in the range [low, high] given parameter array p.
double integrate(const char *funcstr, double *p, double low, double high){
	TF1 *func = new TF1("func", funcstr, low, high);
	func->SetParameters(p);
	double retval = func->Integral(low, high);
	delete func;
	return retval;
}

// Calculate a 2nd order polynomial that passes through three (x,y) pairs.
void calculateP2(double *x, double *y, double *p){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		x1[i] = x[i];
		x2[i] = std::pow(x[i], 2);
	}

	double denom = (x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(x2[2]-x2[1]*1) + x2[0]*(x1[2]-x1[1]*1);

	p[0] = (float)((y[0]*(x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(y[1]*x2[2]-x2[1]*y[2]) + x2[0]*(y[1]*x1[2]-x1[1]*y[2]))/denom);
	p[1] = (float)(((y[1]*x2[2]-x2[1]*y[2]) - y[0]*(x2[2]-x2[1]*1) + x2[0]*(y[2]-y[1]*1))/denom);
	p[2] = (float)(((x1[1]*y[2]-y[1]*x1[2]) - x1[0]*(y[2]-y[1]*1) + y[0]*(x1[2]-x1[1]*1))/denom);
}

// Return the total run time in seconds.
double summation(TTree *t){
	double tdiff;
	double sum = 0;

	t->SetBranchAddress("tdiff", &tdiff);
	
	for(int i = 0; i < t->GetEntries(); i++){
		t->GetEntry(i);
		if(tdiff > 0)
			sum += tdiff;
	}
	
	return sum*8*1E-9;
}

// Return the total number of counts in a 1-d histogram.
//  NOTE: This function returns the number of counts in a histogram, not the integral.
double summation(TH1 *h){
	return h->Integral();
}

// Return the number of counts in a 1-d histogram in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts in a histogram, not the integral.
double summation(TH1 *h, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	for(int i = lowBin; i <= highBin; i++)
		retval += h->GetBinContent(i);
	return retval;
}

// Return the total number of counts under a TF1.
//  NOTE: This function returns the number of counts under a curve, not the function integral.
double summation(TH1 *h, TF1 *f){
	double retval = 0;
	double xlo, xhi;
	for(int i = 1; i <= h->GetNbinsX(); i++){
		xlo = h->GetBinLowEdge(i);
		xhi = xlo + h->GetBinWidth(i);
		retval += f->Integral(xlo, xhi)/(xhi-xlo);
	}
	return retval;
}

// Return the number of counts under a TF1 in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts under a curve, not the function integral.
double summation(TH1 *h, TF1 *f, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	double xlo, xhi;
	for(int i = lowBin; i <= highBin; i++){
		xlo = h->GetBinLowEdge(i);
		xhi = xlo + h->GetBinWidth(i);
		retval += f->Integral(xlo, xhi)/(xhi-xlo);
	}
	return retval;
}

// Multiply a 1-d histogram by a constant.
void multiply(TH1 *h_, const double &c_){
	for(int i = 1; i <= h_->GetNbinsX(); i++){
		h_->SetBinContent(i, h_->GetBinContent(i)*c_);
	}
}

// Save TObject to output root file.
void saveObject(const char *fname, TObject *obj, const std::string &name=""){
	TFile *f = new TFile(fname, "UPDATE");
	f->cd();
	
	if(name.empty())
		obj->Write(obj->GetName());
	else
		obj->Write(name.c_str());

	f->Close();
	delete f;
}

// Multiply a 2-d histogram by a constant.
void scale(TH2F *h, const double &scaling){
	int gbin;
	for(int i = 1; i <= h->GetXaxis()->GetNbins(); i++){
		for(int j = 1; j <= h->GetYaxis()->GetNbins(); j++){
			gbin = h->GetBin(i, j);
			h->SetBinContent(gbin, h->GetBinContent(gbin)*scaling);
		}
	}
}

// List all bins of a 1-d histogram.
void listBins(TH1 *h_, const double &c_=1){
	double sum = 0;
	for(int i = 1; i <= h_->GetNbinsX(); i++){
		sum += h_->GetBinContent(i)*c_;
		std::cout << i << "\t" << h_->GetBinContent(i)*c_ << std::endl;
	}
	std::cout << " total = " << sum << std::endl;
}

// Add a second x-axis to a TCanvas.
TGaxis *extraXaxis(){
	TGaxis *axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUxmax(),gPad->GetUymax(),0,100,510,"+L");
	axis->Draw();
	return axis;
}

// Add a second y-axis to a TCanvas.
TGaxis *extraYaxis(){
	TGaxis *axis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymax(),0,100,510,"+L");
	axis->Draw();
	return axis;
}

TObject *getObject(TFile *f, const std::string &name_, const std::string &dir_=""){
	TObject *ptr = f->Get((dir_+name_).c_str());
	if(!ptr) std::cout << " Error! Failed to find \"" << dir_ << "/" << name_ << "\" in input file.\n";
	return ptr;
}

bool getTNamed(TFile *f, const std::string &name_, double &val, const std::string &dir_=""){
	TNamed *ptr = (TNamed*)getObject(f, name_, dir_);
	if(!ptr) return false;
	val = strtod(ptr->GetTitle(), NULL);
	return true;
}

// Set an object which inherits from TAttLine to have user specified style.
void setLine(TAttLine *ptr_, const short &color_=602, const short &style_=1, const short &width_=1){
	if(!ptr_) return;
	ptr_->SetLineColor(color_);
	ptr_->SetLineStyle(style_);
	ptr_->SetLineWidth(width_);
}

// Set an object which inherits from TAttMarker to have user specified style.
void setMarker(TAttMarker *ptr_, const short &color_=602, const short &style_=21, const float &size_=1.0){
	if(!ptr_) return;
	ptr_->SetMarkerColor(color_);
	ptr_->SetMarkerStyle(style_);
	ptr_->SetMarkerSize(size_);
}

void help(const std::string &search_=""){
	// Colored terminal character string.
	const std::string dkred("\033[1;31m");
	const std::string dkblue("\033[1;34m");
	const std::string reset("\033[0m");

	// Defined global constants.
	const std::string globalConstants[12] = {"const double", "Mn", "Mass of neutron, in MeV.",
	                                         "const double", "pi", "pi constant.",
	                                         "const double", "twopi", "2*pi constant.",
	                                         "const short", "colors[6]", "A`rray of root colors."};

	// Defined functions.
	const std::string definedFunctions[88] = {"void", "calculateP2", "double *x, double *y, double *p", "Calculate a 2nd order polynomial that passes through three (x,y) pairs.",
	                                          "void", "conv", "double &x, double &y", "Convert logic signal period to beam current (enA).",
	                                          "TGraph", "convert", "TGraph *g, const char *name", "Convert a logic signal TGraph to a beam current vs. time graph.",
	                                          "TGaxis", "extraXaxis", "", "Add a second x-axis to a TCanvas.",
	                                          "TGaxis", "extraYaxis", "", "Add a second y-axis to a TCanvas.",
	                                          "void", "findBins", "TH1 *h, const double &xstart_, const double &xstop_, int &lowBin, int &highBin", "Return the range of bins containing an upper and lower point, rounded to the nearest bins.",
	                                          "double", "integrate", "TGraph *g", "Return the total integral of a logic signal TGraph.",
	                                          "double", "integrate", "TH1 *h", "Return the total integral of a 1-d histogram.",
	                                          "double", "integrate", "TH1 *h, const double &xstart_, const double &xstop_", "Return the integral of a 1-d histogram in the range [low, high], rounded to the nearest bins.",
	                                          "double", "integrate", "const char *funcstr, double *p, double low, double high", "Integrate a root function in the range [low, high] given parameter array p.",
	                                          "void", "listBins", "TH1 *h_, const double &c_=1", "List all bins of a 1-d histogram.",
	                                          "double", "mean", "TGraph *g", "Determine the mean beam current from a logic signal TGraph.",
	                                          "void", "multiply", "TH1 *h_, const double &c_", "Multiply a 1-d histogram by a constant.",
                                                  "void", "saveObject", "TFile *ouf, TObject *obj, const std::string &name=\"\"", "Save TObject to output file.",
	                                          "void", "scale", "TH2F *h, const double &scaling", "Multiply a 2-d histogram by a constant.",
	                                          "void", "setLine", "TAttLine *ptr_, const short &color_=602, const short &style_=1, const short &width_=1", "Set an object which inherits from TAttLine to have user specified style.",
	                                          "void", "setMarker", "TAttMarker *ptr_, const short &color_=602, const short &style_=21, const float &size_=1.0", "Set an object which inherits from TAttMarker to have user specified style.",
	                                          "double", "summation", "TTree *t", "Return the total run time in seconds.",
	                                          "double", "summation", "TH1 *h", "Return the total number of counts in a 1-d histogram.",
	                                          "double", "summation", "TH1 *h, const double &xstart_, const double &xstop_", "Return the number of counts in a 1-d histogram in the range [low, high], rounded to the nearest bins.",
	                                          "double", "summation", "TH1 *h, TF1 *f", "Return the total number of counts under a TF1.",
	                                          "double", "summation", "TH1 *h, TF1 *f, const double &xstart_, const double &xstop_", "Return the number of counts under a TF1 in the range [low, high], rounded to the nearest bins."};

	if(search_.empty()){
		std::cout << "*****************************\n";
		std::cout << "**          HELP           **\n";
		std::cout << "*****************************\n";
	
		std::cout << " Defined global constants:\n";
		for(int i = 0; i < 4; i++)
			std::cout << "  " << globalConstants[3*i+1] << std::endl;
	
		std::cout << "\n Defined helper functions:\n";
		for(int i = 0; i < 22; i++)
			std::cout << "  " << definedFunctions[4*i+1] << std::endl;
			
		std::cout << std::endl;
	}
	else{
		size_t fIndex;
		std::string strings[3];
		
		for(int i = 0; i < 4; i++){
			fIndex = globalConstants[3*i+1].find(search_);
			if(fIndex != std::string::npos){
				strings[0] = globalConstants[3*i+1].substr(0, fIndex);
				strings[1] = globalConstants[3*i+1].substr(fIndex, search_.length());
				strings[2] = globalConstants[3*i+1].substr(fIndex+search_.length());
				std::cout << "  " << globalConstants[3*i];
				std::cout << " " << strings[0] << dkred << strings[1] << reset << strings[2];
				std::cout << dkblue << " //" << globalConstants[3*i+2] << reset << "\n";
			}
		}
	
		for(int i = 0; i < 22; i++){
			fIndex = definedFunctions[4*i+1].find(search_);
			if(fIndex != std::string::npos){
				strings[0] = definedFunctions[4*i+1].substr(0, fIndex);
				strings[1] = definedFunctions[4*i+1].substr(fIndex, search_.length());
				strings[2] = definedFunctions[4*i+1].substr(fIndex+search_.length());
				std::cout << "  " << definedFunctions[4*i];
				std::cout << " " << strings[0] << dkred << strings[1] << reset << strings[2];
				std::cout << " (" << definedFunctions[4*i+2] << ")";
				std::cout << dkblue << " //" << definedFunctions[4*i+3] << reset << "\n";
			}
		}
	}
}

int helper(){
	std::cout << " helper.cpp\n";
	std::cout << "  NOTE - type 'help()' to display a list of commands.\n";
	std::cout << "  NOTE - or 'help(string)' to search for a command or variable.\n";
	return 0;
}
