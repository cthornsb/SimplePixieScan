#include <iostream>
#include <sstream>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>

#include "TApplication.h"
#include "TSystem.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"
#include "TCutG.h"

#include "CTerminal.h"
#include "optionHandler.hpp"

#include "simpleTool.hpp"

#define USLEEP_WAIT_TIME 1E4 // = 0.01 seconds

const double pi = 3.1415926536;
const double cvac = 29.9792458; // cm/ns
const double Mn = 10454.0750977429; // MeV

// Return the neutron TOF for an energy in MeV.
double tof2energy(const double &tof_, const double &d_){
	return (0.5*Mn*d_*d_/(tof_*tof_));
}

// Return the neutron energy for a TOF in ns.
double energy2tof(const double &E_, const double &d_){
	return (d_*std::sqrt(Mn/(2*E_)));
}

// Add dtheta_ to theta and wrap between 0 and 2pi.
void addAngles(double &theta, const double &dtheta_){
	theta += dtheta_;
	if(theta < 0) theta += 2*pi;
	else if(theta > 2*pi) theta -= 2*pi;
}

// Return a random number between low and high.
double frand(double low, double high){
	return low+(double(rand())/RAND_MAX)*(high-low);
}

// Convert spherical coordinates to cartesian.
void sphere2Cart(const double &r_, const double &theta_, const double &phi_, double &x, double &y, double &z){ 
	x = r_*std::sin(theta_)*std::cos(phi_);
	y = r_*std::sin(theta_)*std::sin(phi_);
	z = r_*std::cos(theta_);
} 

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, std::string &left, std::string &right, const char &delim/*=':'*/){
	size_t index = str.find(delim);
	if(index != std::string::npos){
		left = str.substr(0, index);
		right = str.substr(index+1);
		return true;
	}
	return false;
}

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, double &left, double &right, const char &delim/*=':'*/){
	size_t index = str.find(delim);
	if(index != std::string::npos){
		left = strtod(str.substr(0, index).c_str(), NULL);
		right = strtod(str.substr(index+1).c_str(), NULL);
		return true;
	}
	return false;
}

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, int &left, int &right, const char &delim/*=':'*/){
	size_t index = str.find(delim);
	if(index != std::string::npos){
		left = strtol(str.substr(0, index).c_str(), NULL, 0);
		right = strtol(str.substr(index+1).c_str(), NULL, 0);
		return true;
	}
	return false;
}

// Split a filename into path and extension.
bool splitFilename(const std::string &str, std::string &left, std::string &right, const char &delim/*='.'*/){
	size_t index = str.find_last_of(delim);
	if(index != std::string::npos){
		left = str.substr(0, index);
		right = str.substr(index);
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// class interpolator
///////////////////////////////////////////////////////////////////////////////

interpolator::interpolator(){ }

interpolator::interpolator(const char *fname_){
	load(fname_);
}

bool interpolator::load(const char *fname_){
	std::ifstream ifile(fname_);
	
	if(!ifile.good()) return false;

	double x, y;
	while(true){
		ifile >> x >> y;
		if(ifile.eof()) break;

		xvals.push_back(x);
		yvals.push_back(y);
	}
	
	ifile.close();

	return !xvals.empty();
}

bool interpolator::interpolate(const double &x_, double &y){
	for(size_t i = 1; i < xvals.size(); i++){
		if(xvals[i-1] <= x_ && xvals[i] > x_){
			double x1 = xvals[i-1]; double x2 = xvals[i];
			double y1 = yvals[i-1]; double y2 = yvals[i];
			y = y1 + (x_-x1)*(y2-y1)/(x2-x1);
			return true;
		}
	}
	return false;
}

double interpolator::interpolate(const double &x_){
	double dummy;
	if(interpolate(x_, dummy))
		return dummy;
	return -9999;
}

///////////////////////////////////////////////////////////////////////////////
// class progressBar
///////////////////////////////////////////////////////////////////////////////

void progressBar::start(const long long &numEntries_){
	if(numEntries_ < length) // Probably will rarely happen, but just to be safe.
		length = numEntries_;
	progStr = std::string(length, ' ');
	numEntries = numEntries_;
	chunkSize = numEntries / length;
	chunkCount = 0;
	std::cout << "\n Processing " << numEntries << " events.\n";
	std::cout << "  Working - 0% [" << progStr << "] 100%\r" << std::flush;
}

void progressBar::check(const long long &entry_){
	if(entry_ % chunkSize == 0 && entry_ != 0){
		progStr[chunkCount] = '=';
		std::cout << "  Working - 0% [" << progStr << "] 100%\r" << std::flush;
		chunkCount++;
	}
}

void progressBar::finalize(){
	std::cout << "  Working - 0% [" << std::string(length, '=') << "] 100% Done!\n";
}

///////////////////////////////////////////////////////////////////////////////
// class simpleTool
///////////////////////////////////////////////////////////////////////////////

simpleTool::simpleTool(){
	rootapp = NULL;
	
	can1 = NULL;
	can2 = NULL;
	
	infile = NULL;
	outfile = NULL;
	
	intree = NULL;
	outtree = NULL;

	tcutg = NULL;	

	input_objname = "data";
	output_filename = "";
	output_objname = "";
	cut_filename = "";

	debug = false;	
	filledFromTree = false;

	start_entry = 0;
	entries_to_process = 0;
	max_entries_to_process = -1;
	current_entry = 0;
	
	baseOpts.push_back(optionExt("help", no_argument, NULL, 'h', "", "Display this dialogue."));
	baseOpts.push_back(optionExt("input", required_argument, NULL, 'i', "<filename>", "Specifies an input file to analyze."));
	baseOpts.push_back(optionExt("output", required_argument, NULL, 'o', "<filename>", "Specifies the name of the output file."));
	baseOpts.push_back(optionExt("name", required_argument, NULL, 'n', "<name>", "Specify the name of the input TTree or TH1."));
	baseOpts.push_back(optionExt("tcutg", required_argument, NULL, 'C', "<filename:cutname>", "Specify the name of the TCutG input file and the name of the cut."));
	baseOpts.push_back(optionExt("multi", required_argument, NULL, 'm', "<N>", "Specify multiple input filenames e.g. filename.root, filename-1.root, ..., filename-N.root."));
	baseOpts.push_back(optionExt("start", required_argument, NULL, 's', "<N>", "Specify the start entry in the input tree (default=0)."));
	baseOpts.push_back(optionExt("entries", required_argument, NULL, 'e', "<N>", "Specify the number of entries to process from the input tree."));

	optstr = "hi:o:n:C:m:s:e:";

	// Get the current working directory.
	char workingDirectory[1024];
	work_dir = std::string(getcwd(workingDirectory, 1024));
	work_dir += '/';

	// Get the home directory.
	home_dir = std::string(getenv("HOME"));
	home_dir += '/';
}

simpleTool::~simpleTool(){
	if(infile){
		infile->Close();
		delete infile;
	}
	if(outfile){
		outfile->Close();
		delete outfile;
	}
	if(can1){
		can1->Close();
		delete can1;
	}
	if(can2){
		can2->Close();
		delete can2;
	}
}

/** Get the specified entry from the input TTree. Does not update the progress bar.
  * \param[in]  entry_ The entry to get from the TTree.
  * \return True if the TTree is loaded and the specified entry exists and return false otherwise.
  */
bool simpleTool::getEntry(const long long &entry_){
	if(!intree) return false;
	return (intree->GetEntry(entry_) > 0);
}

/** Get the next entry from the input TTree.
  * \return True if the TTree is loaded, the next entry exists, and the max number of entries has not been reached. Returns false otherwise.
  */
bool simpleTool::getNextEntry(){
	if(!intree || current_entry >= start_entry+entries_to_process) return false;
	pbar.check(current_entry);
	if(current_entry+1 >= start_entry+entries_to_process) 
		pbar.finalize();
	return (intree->GetEntry(current_entry++) > 0);
}

/** Get the full pathname from an input string.
  * \param[in]  path_ The user specified filename. May be relative or absolute.
  * \return String containing the full path to the input file.
  */
std::string simpleTool::getRealPath(const std::string &path_){
	full_input_filename = path_;

	if(path_.find('~') != std::string::npos)
		full_input_filename.replace(path_.find('~'), 1, home_dir);

	char *realPathOutput = realpath(full_input_filename.c_str(), NULL);
	if(realPathOutput){
		full_input_filename = std::string(realPathOutput);
		free(realPathOutput);
	}

	return full_input_filename;
}

/** Print a command line argument help dialogue.
  * \param[in]  name_ The name of the program.
  * \return Nothing.
  */  
void simpleTool::help(char *name_){
	std::cout << " usage: " << name_ << " [options]\n";
	std::cout << "  Available options:\n";
	for(std::vector<optionExt>::iterator iter = baseOpts.begin(); iter != baseOpts.end(); iter++){
		if(!iter->name) continue;
		iter->print(40, "   ");
	}
	for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
		if(!iter->name) continue;
		iter->print(40, "   ");
	}
}

bool simpleTool::setup(int argc, char *argv[]){
	this->addOptions();

	// Build the vector of all command line options.
	for(std::vector<optionExt>::iterator iter = baseOpts.begin(); iter != baseOpts.end(); iter++){
		longOpts.push_back(iter->getOption()); 
	}
	for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
		longOpts.push_back(iter->getOption()); 
	}
	
	// Append all zeros onto the option list. Required for getopt_long.
	struct option zero_opt { 0, 0, 0, 0 };
	longOpts.push_back(zero_opt);
	
	int idx = 0;
	int retval = 0;
	
	int largestFileIndex = -1;
	bool multiFileMode = false;

	//getopt_long is not POSIX compliant. It is provided by GNU. This may mean
	//that we are not compatable with some systems. If we have enough
	//complaints we can either change it to getopt, or implement our own class. 
	while ( (retval = getopt_long(argc, argv, optstr.c_str(), longOpts.data(), &idx)) != -1) {
		if(retval == 0x0){ // Long option
			for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
				if(strcmp(iter->name, longOpts[idx].name) == 0){
					iter->active = true;
					if(optarg)
						iter->argument = std::string(optarg);
					break;
				}
			}
		}
		else if(retval == 0x3F){ // Unknown option, '?'
			return false;
		}
		else{ // Single character option.
			switch(retval) {
				case 'h' :
					help(argv[0]);
					return false;
				case 'i' :
					input_filename = optarg;
					filename_list.push_back(optarg);
					break;
				case 'o' :
					output_filename = optarg;
					break;
				case 'n' :
					input_objname = optarg;
					break;
				case 'C' :
					cut_filename = optarg;
					break;
				case 'm' :
					largestFileIndex = strtol(optarg, NULL, 0);
					multiFileMode = true;
					break;
				case 's' :
					start_entry = strtoll(optarg, NULL, 0);
					if(start_entry < 0){
						std::cout << " Error: Invalid starting TTree entry (" << start_entry << ")!\n";
						return false;
					}
					break;
				case 'e' :
					max_entries_to_process = strtoll(optarg, NULL, 0);
					if(max_entries_to_process <= 0){
						std::cout << " Error: Invalid number of entries to process (" << max_entries_to_process << ")!\n";
						return false;
					}
					break;
				default:
					for(std::vector<optionExt>::iterator iter = userOpts.begin(); iter != userOpts.end(); iter++){
						if(retval == iter->val){
							iter->active = true;
							if(optarg)
								iter->argument = std::string(optarg);
							break;
						}
					}
					break;
			}
		}
	}//while

	if(multiFileMode){
		if(input_filename.empty()){
			std::cout << " Error: Input filename not specified, needed for --multi!\n";
			return false;
		}
		if(largestFileIndex < 1){
			std::cout << " Error: Final filename index must be greater than or equal to 1, user entered (" << largestFileIndex << ")!\n";
			return false;
		}
		
		std::string prefix, suffix;
		if(!splitFilename(input_filename, prefix, suffix)){
			prefix = input_filename;
			suffix = ".root";
		}
		
		for(int i = 1; i <= largestFileIndex; i++){
			std::stringstream stream;
			stream << prefix << "-" << i << suffix;
			filename_list.push_back(stream.str().c_str());
		}
	}

	if(!processArgs()) return false;
	
	return true;
}

TApplication *simpleTool::initRootGraphics(){
	if(!rootapp) rootapp = new TApplication("rootapp", 0, NULL);
	return rootapp;
}

TCanvas *simpleTool::openCanvas1(const std::string &title_/*="Canvas"*/){
	initRootGraphics();

	can1 = new TCanvas("can1", title_.c_str());
	can1->cd();
	
	return can1;
}

TCanvas *simpleTool::openCanvas2(const std::string &title_/*="Canvas"*/){
	initRootGraphics();

	can2 = new TCanvas("can2", title_.c_str());
	can2->cd();
	
	return can2;
}

TFile *simpleTool::openInputFile(){
	if(filename_list.empty()) return NULL;
	if(infile != NULL && infile->IsOpen()){
		infile->Close();
		delete infile;
		intree = NULL;	
	}
	input_filename = filename_list.front();
	full_input_filename = getRealPath(input_filename);
	filename_list.pop_front();
	infile = new TFile(input_filename.c_str(), "READ");
	if(!infile->IsOpen()){
		std::cout << " Error! Failed to open input file '" << input_filename << "'.\n";
		return NULL;
	}
	return infile;
}

TTree *simpleTool::loadInputTree(){
	intree = (TTree*)infile->Get(input_objname.c_str());
	if(!intree){
		std::cout << " Error! Failed to load input TTree '" << input_objname << "'.\n";
		return NULL;
	}
	if(intree->GetEntries() == 0){
		std::cout << " Error! Input tree has zero entries.\n";
		return NULL;
	}
	if(max_entries_to_process <= 0) // No user specified number of entries.
		entries_to_process = intree->GetEntries();
	else // The user has specified a maximum number of entries to read.
		entries_to_process = (intree->GetEntries() > max_entries_to_process ? max_entries_to_process : intree->GetEntries());
	current_entry = start_entry;
	pbar.start(entries_to_process);
	return intree;
}

TFile *simpleTool::openOutputFile(){
	if(outfile != NULL && outfile->IsOpen()){
		outfile->Close();
		delete outfile;
		outtree = NULL;	
	}
	outfile = new TFile(output_filename.c_str(), "RECREATE");
	if(!outfile->IsOpen()){
		std::cout << " Error! Failed to open input file '" << output_filename << "'.\n";
		return NULL;
	}
	return outfile;
}

TCutG *simpleTool::loadTCutG(){
	if(cut_filename.empty()) return NULL;
	std::string fname, cname;
	if(!splitByColon(cut_filename, fname, cname)){
		std::cout << " Error: Name of TCutG not specified. Expected <filename:cutname> e.g. myfile.root:cut1.\n";
		return NULL;
	}
	TFile *cutFile = new TFile(fname.c_str(), "READ");
	if(!cutFile->IsOpen()){
		std::cout << " Error: Failed to load cut file \"" << fname << "\".\n";
		return NULL;
	}
	tcutg = (TCutG*)cutFile->Get(cname.c_str());
	if(!tcutg){
		std::cout << " Error: Failed to load input TCutG \"" << cname << "\".\n";
		cutFile->Close();
		return NULL;
	}
	tcutg = (TCutG*)tcutg->Clone("simple_tcutg");
	cutFile->Close();
	return tcutg;
}	

void simpleTool::wait(){
	setup_signal_handlers();
	int signalReturn; 
	std::cout << " Press ctrl^c or ctrl^z to exit.\n";
	while(true){
		signalReturn = check_signals();
		if(signalReturn == SIGSEGV || signalReturn == SIGINT || signalReturn == SIGTSTP) 
			break;
		gSystem->ProcessEvents();
		usleep(USLEEP_WAIT_TIME);
	}
	if(signalReturn == SIGSEGV) std::cout << " Segmentation fault!\n";
	else if(signalReturn == SIGINT) std::cout << " Received SIGINT (ctrl^c) signal.\n";
	else if(signalReturn == SIGTSTP) std::cout << " Received SIGTSTP (ctrl^z) signal.\n";
	unset_signal_handlers();
}

///////////////////////////////////////////////////////////////////////////////
// class simpleHistoFitter
///////////////////////////////////////////////////////////////////////////////

simpleHistoFitter::simpleHistoFitter() : simpleTool() {
	h1d = NULL;
	h2d = NULL;

	nEntries = 1E6;

	draw_string = "";
	expr_string = "";
	
	useProjX = true;

	firstChildOption = 0;
	
	histStartID = 0;
	histStopID = -1;
}

bool simpleHistoFitter::getProjectionX(TH1 *h1_, TH2 *h2_, const int &binY_){
	// Check that the histograms are defined.
	if(!h1_ || !h2_) return false;

	// Check for index out of range.
	if(binY_ < 0 || binY_ > h2_->GetNbinsY()) return false;
	
	// Check that both histograms have the same number of bins in the x-axis.
	if(h1_->GetNbinsX() != h2_->GetNbinsX()) return false;
	
	// Empty the output histogram.
	h1_->Reset();

	// Fill the output histogram.
	double total = 0.0;
	double binContent;
	for(int i = 1; i <= h2_->GetNbinsX(); i++){
		binContent = h2_->GetBinContent(h2_->GetBin(i, binY_));
		h1_->SetBinContent(i, binContent);
		total += binContent;
	}
	
	return (total > 0.0);
}

bool simpleHistoFitter::getProjectionY(TH1 *h1_, TH2 *h2_, const int &binX_){
	// Check that the histograms are defined.
	if(!h1_ || !h2_) return false;

	// Check for index out of range.
	if(binX_ < 0 || binX_ > h2_->GetNbinsX()) return false;

	// Check that both histograms have the same number of bins in the y-axis.
	if(h1_->GetNbinsX() != h2_->GetNbinsY()) return false;
	
	// Empty the output histogram.
	h1_->Reset();

	// Fill the output histogram.
	double total = 0.0;
	double binContent;
	for(int i = 1; i <= h2_->GetNbinsY(); i++){
		binContent = h2_->GetBinContent(h2_->GetBin(binX_, i));
		h1_->SetBinContent(i, binContent);
		total += binContent;
	}
	
	return (total > 0.0);
}

bool simpleHistoFitter::getProjection(TH1 *h1_, TH2 *h2_, const int &bin_){
	if(bin_ < histStartID || (histStopID >= 0 && bin_ > histStopID)) return false;
	if(useProjX) return getProjectionX(h1_, h2_, bin_);
	return getProjectionY(h1_, h2_, bin_);
}

TH1D *simpleHistoFitter::getProjectionHist(TH2 *h2_, const char *name_/*="h1"*/, const char *title_/*=""*/){
	// Check that the histogram is defined.
	if(!h2_) return NULL;	
	TH1D *output = NULL;
	if(useProjX){
		int nBins = h2d->GetNbinsX();
		double *bins = new double[nBins+1];
		for(int i = 1; i <= nBins+1; i++){
			bins[i-1] = h2d->GetXaxis()->GetBinLowEdge(i);
		}
		output = new TH1D(name_, title_, nBins, bins);
	}
	else{
		int nBins = h2d->GetNbinsY();
		double *bins = new double[nBins+1];
		for(int i = 1; i <= nBins+1; i++){
			bins[i-1] = h2d->GetYaxis()->GetBinLowEdge(i);
		}
		output = new TH1D(name_, title_, nBins, bins);
	}
	return output;
}

int simpleHistoFitter::getNumProjections(TH2 *h2_){
	// Check that the histogram is defined.
	if(!h2_) return -1;
	if(useProjX) return h2_->GetNbinsY();
	return h2_->GetNbinsX();
}

double simpleHistoFitter::getBinLowEdge(TH2 *h2_, const int &bin_){
	// Check that the histogram is defined.
	if(!h2_) return -1;
	if(useProjX) return h2_->GetYaxis()->GetBinLowEdge(bin_);
	return h2_->GetXaxis()->GetBinLowEdge(bin_);
}

double simpleHistoFitter::getBinWidth(TH2 *h2_, const int &bin_){
	// Check that the histogram is defined.
	if(!h2_) return -1;
	if(useProjX) return h2_->GetYaxis()->GetBinWidth(bin_);
	return h2_->GetXaxis()->GetBinWidth(bin_);
}

double simpleHistoFitter::getMaximum(TH1 *h1_, const double &lowVal_, const double &highVal_, double &mean){
	double maximum = -9999;
	int lowBin = h1_->FindBin(lowVal_);
	int highBin = h1_->FindBin(highVal_);
	for(int i = lowBin; i <= highBin; i++){
		if(h1_->GetBinContent(i) > maximum){
			maximum = h1_->GetBinContent(i);
			mean = h1_->GetBinCenter(i);
		}
	}
	return maximum;
}

void simpleHistoFitter::addOptions(){
	addOption(optionExt("draw", required_argument, NULL, 0, "<drawstr>", "Root draw string to fill the 2d histogram."), userOpts, optstr);
	addOption(optionExt("expr", required_argument, NULL, 0, "<exprstr>", "Root expression logic string to use for filling 2d histogram."), userOpts, optstr);
	addOption(optionExt("save", optional_argument, NULL, 0, "[name]", "Write the 2d histogram to the output file."), userOpts, optstr);
	addOption(optionExt("debug", no_argument, NULL, 'd', "", "Enable debug mode."), userOpts, optstr);
	addOption(optionExt("y-axis", no_argument, NULL, 'y', "", "Project along the y-axis instead of the x-axis."), userOpts, optstr);
	addOption(optionExt("range", required_argument, NULL, 'r', "<range>", "Process a range of bins from the input histogram (specify range as start:stop e.g. 15:21)."), userOpts, optstr);
	addOption(optionExt("entries", required_argument, NULL, 'N', "<entries>", "Fille histogram only from the first N entries from the input file (default = 1E6)."), userOpts, optstr);

	// Add derived classes to the vector of all command line options.
	firstChildOption = userOpts.size();
	addChildOptions();
}

bool simpleHistoFitter::processArgs(){
	if(userOpts.at(0).active)
		draw_string = userOpts.at(0).argument;
	if(userOpts.at(1).active)
		expr_string = userOpts.at(1).argument;
	if(userOpts.at(2).active)
		output_objname = userOpts.at(2).argument;
	if(userOpts.at(3).active)
		debug = true;
	if(userOpts.at(4).active)
		useProjX = false;
	if(userOpts.at(5).active){
		if(!splitByColon(userOpts.at(5).argument, histStartID, histStopID)){ 
			histStartID = strtol(userOpts.at(5).argument.c_str(), 0, 0);
			histStopID = histStartID;
		}
	}
	if(userOpts.at(6).active)
		nEntries = strtoul(userOpts.at(6).argument.c_str(), NULL, 0);

	return processChildArgs();
}

TH2 *simpleHistoFitter::fillHistogram(){
	if(draw_string.empty()){
		h2d = (TH2*)infile->Get(input_objname.c_str());
		if(!h2d){
			std::cout << " Error! Failed to load input histogram '" << input_objname << "'.\n";
			return NULL;
		}
	}
	else{
		if(!intree && !loadInputTree())
			return NULL;

		std::cout << " " << input_objname << "->Draw(\"" << draw_string << "\", \"" << expr_string << "\", \"COLZ\", " << nEntries << ");\n";
		std::cout << " Filling TH2... " << std::flush;
		if(intree->Draw(draw_string.c_str(), expr_string.c_str(), "COLZ", nEntries) > 0){
			std::cout << "DONE\n";
		}
		else{
			std::cout << "FAILED\n";
			return NULL;
		}
	
		h2d = (TH2*)(intree->GetHistogram()->Clone("h2d"));

		filledFromTree = true;
	}

	if(!output_filename.empty()){
		if(!outfile)
			openOutputFile();
		outfile->cd();
		if(!output_objname.empty()){
			h2d->Write(output_objname.c_str());
			std::cout << " Saved histogram '" << output_objname << "' to file '" << output_filename << "'.\n";
		}	
		else{
			h2d->Write();
			std::cout << " Saved histogram '" << h2d->GetName() << "' to file '" << output_filename << "'.\n";
		}
	}
	
	return h2d;
}

int simpleHistoFitter::execute(int argc, char *argv[], bool drawh2_/*=false*/){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}
	
	if(!openInputFile())
		return 2;

	if(!fillHistogram())
		return 3;
	
	if(drawh2_ || filledFromTree){
		openCanvas1();
		can1->SetLogz();
		can1->cd();
		
		h2d->Draw("COLZ");
		can1->Update();
		can1->WaitPrimitive();
	}

	openCanvas2();

	if(!process())
		return 4;
	
	return 0;
}
