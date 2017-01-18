#include <stdlib.h>

#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TH2I.h"

#include "simpleTool.hpp"
#include "Structures.h"

#define ADC_CLOCK 4 // ns per ADC clock tick

class tracer : public simpleTool {
  private:
  	long long startEntry;
  	long long numEntries;
	unsigned int tlength;
 
	Trace *trace;

	TBranch *branch;
 	
	TH2I *hist;

	bool setAddresses();

	void getEntry();

  public:
	tracer();

	~tracer();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

bool tracer::setAddresses(){
	if(!intree) return false;

	intree->SetBranchAddress("trace", &trace, &branch);
	
	return (branch != NULL);
}

void tracer::getEntry(){
	if(!intree) return;

	long long stopEntry = startEntry+numEntries;
	if(stopEntry > intree->GetEntries()) stopEntry = intree->GetEntries();
	
	bool firstEntry = true;
	for(long long entry = startEntry; entry < stopEntry; entry++){
		intree->GetEntry(entry);

		// Get the trace length.
		if(firstEntry){
			tlength = trace->wave.size()/trace->mult;
			std::cout << " Trace length is " << tlength << " ADC ticks (" << tlength*ADC_CLOCK << " ns).\n";
			hist = new TH2I("hist", "Traces", tlength, 0, tlength*ADC_CLOCK, numEntries, 0, numEntries);
			firstEntry = false;
		}
	
		int bin;
		for(unsigned int i = 0; i < tlength; i++){
			bin = hist->GetBin(i, entry-startEntry);
			hist->SetBinContent(bin, trace->wave.at(i));
		}
	}
}

tracer::tracer() : simpleTool(), startEntry(0), numEntries(1), tlength(0), trace(NULL), branch(NULL), hist(NULL) { 
	input_objname = "trace";
}

tracer::~tracer(){
	if(hist) delete hist;
}

void tracer::addOptions(){
	addOption(optionExt("start", required_argument, NULL, 's', "<start-entry>", "Specify the first tree entry."), userOpts, optstr);
	addOption(optionExt("number", required_argument, NULL, 'N', "<num-entries>", "Specify the number of entries to read."), userOpts, optstr);
}

bool tracer::processArgs(){
	if(userOpts.at(0).active){
		startEntry = strtoll(userOpts.at(0).argument.c_str(), NULL, 0);
		if(startEntry < 0){
			std::cout << " Error: User specified illegal start entry (" << startEntry << ")!\n";
			return false;
		}
	}
	if(userOpts.at(1).active){
		numEntries = strtoll(userOpts.at(1).argument.c_str(), NULL, 0);
		if(numEntries < 0){
			std::cout << " Error: User specified illegal number of entries (" << numEntries << ")!\n";
			return false;
		}
	}
		
	return true;
}

int tracer::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(!output_filename.empty()){
		if(!openOutputFile()){
			std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
			return 2;
		}
	}

	if(!openInputFile()){
		std::cout << " Error: Failed to load input file \"" << input_filename << "\".\n";
		return 3;
	}
	
	if(!loadInputTree()){
		std::cout << " Error: Failed to load TTree \"" << input_objname << "\".\n";
		return 4;
	}

	if(!setAddresses()){
		std::cout << " Error: Failed to set branch addresses of \"" << input_objname << "\".\n";
		return 5;
	}

	getEntry();
	
	openCanvas1();
	hist->Draw("COLZ");

	// Wait for the user to issue ctrl^c or ctrl^z.
	this->wait();
	
	if(!output_filename.empty()){
		outfile->cd();
		can1->Write("canvas");
		std::cout << " Saved TCanvas to file '" << output_filename << "'.\n";
	}

	return 0;
}

int main(int argc, char *argv[]){
	tracer obj;
	
	return obj.execute(argc, argv);
}
