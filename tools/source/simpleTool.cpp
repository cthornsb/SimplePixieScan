#include <iostream>
#include <sstream>
#include <fstream>

#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TH2.h"

#include "simpleTool.hpp"

///////////////////////////////////////////////////////////////////////////////
// class progressBar
///////////////////////////////////////////////////////////////////////////////

void progressBar::start(const unsigned int &numEntries_){ 
	numEntries = numEntries_;
	chunkSize = numEntries / length;
	std::cout << "\n Processing " << numEntries << " events.\n";
	std::cout << "  Working - 0% [" << progStr << "] 100%\r" << std::flush;
}

void progressBar::check(const unsigned int &entry_){
	if(entry_ % chunkSize == 0 && entry_ != 0){
		progStr[chunkCount] = '=';
		std::cout << "  Working - 0% [" << progStr << "] 100%\r" << std::flush;
		chunkCount++;
	}
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
	
	input_filename = "";
	input_objname = "data";
	output_filename = "";
	output_objname = "";
	
	baseOpts.push_back(optionExt("help", no_argument, NULL, 'h', "", "Display this dialogue."));
	baseOpts.push_back(optionExt("input", required_argument, NULL, 'i', "<filename>", "Specifies the input file to analyze."));
	baseOpts.push_back(optionExt("output", required_argument, NULL, 'o', "<filename>", "Specifies the name of the output file."));
	baseOpts.push_back(optionExt("name", required_argument, NULL, 'n', "<name>", "Specify the name of the input TTree or TH1."));
	
	optstr = "hi:o:n:";
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
					break;
				case 'o' :
					output_filename = optarg;
					break;
				case 'n' :
					input_objname = optarg;
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
	
	if(!processArgs()) return false;
	
	return true;
}

TCanvas *simpleTool::openCanvas1(const std::string &title_/*="Canvas"*/){
	if(!rootapp)
		rootapp = new TApplication("rootapp", 0, NULL);
	
	can1 = new TCanvas("can1", title_.c_str());
	can1->cd();
	
	return can1;
}

TCanvas *simpleTool::openCanvas2(const std::string &title_/*="Canvas"*/){
	if(!rootapp)
		rootapp = new TApplication("rootapp", 0, NULL);
	
	can2 = new TCanvas("can2", title_.c_str());
	can2->cd();
	
	return can2;
}

TFile *simpleTool::openInputFile(){
	if(infile != NULL && infile->IsOpen()){
		infile->Close();
		delete infile;
		intree = NULL;	
	}
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

///////////////////////////////////////////////////////////////////////////////
// class simpleHistoFitter
///////////////////////////////////////////////////////////////////////////////

simpleHistoFitter::simpleHistoFitter() : simpleTool() {
	h1d = NULL;
	h2d = NULL;

	draw_string = "";
	
	debug = false;
}

bool simpleHistoFitter::getProjectionX(TH1 *h1_, TH2 *h2_, const int &binY_){
	// Check that the histograms are defined.
	if(!h1_ || !h2_) return false;

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

bool simpleHistoFitter::getProjectionY(TH1 *h1_, TH2 *h2_, const int &binX_){
	// Check that the histograms are defined.
	if(!h1_ || !h2_) return false;

	// Check for index out of range.
	if(binX_ < 0 || binX_ > h2_->GetXaxis()->GetNbins()) return false;
	
	// Check that both histograms have the same number of bins in the y-axis.
	if(h1_->GetYaxis()->GetNbins() != h2_->GetYaxis()->GetNbins()) return false;
	
	// Empty the output histogram.
	h1_->Reset();

	// Fill the output histogram.
	double total = 0.0;
	double binContent;
	for(int i = 0; i <= h1_->GetYaxis()->GetNbins()+1; i++){
		binContent = h2_->GetBinContent(h2_->GetBin(binX_, i));
		h1_->SetBinContent(binContent, i);
		total += binContent;
	}
	
	return (total > 0.0);
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
	addOption(optionExt("save", optional_argument, NULL, 0, "[name]", "Write the 2d histogram to the output file."), userOpts, optstr);
	addOption(optionExt("debug", no_argument, NULL, 'd', "", "Enable debug mode."), userOpts, optstr);
}

bool simpleHistoFitter::processArgs(){
	if(userOpts.at(0).active)
		draw_string = userOpts.at(0).argument;
	if(userOpts.at(1).active)
		output_objname = userOpts.at(1).argument;
	if(userOpts.at(2).active)
		debug = true;

	return true;
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
	
		std::cout << " " << input_objname << "->Draw(\"" << draw_string << "\", \"\", \"COLZ\");\n";
		std::cout << " Filling TH2... " << std::flush;
		if(intree->Draw(draw_string.c_str(), "", "COLZ") > 0){
			std::cout << "DONE\n";
		}
		else{
			std::cout << "FAILED\n";
			return NULL;
		}
	
		h2d = (TH2*)(intree->GetHistogram()->Clone("h2d"));
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

int simpleHistoFitter::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}
	
	if(!openInputFile())
		return 2;

	openCanvas1();
	can1->SetLogz();
	can1->cd();
		
	if(!fillHistogram())
		return 3;
		
	h2d->Draw("COLZ");
	can1->Update();
	can1->WaitPrimitive();
	openCanvas2();

	if(!process())
		return 4;
	
	return 0;
}
