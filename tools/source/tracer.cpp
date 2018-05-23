#include <stdlib.h>

#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"
#include "TH2I.h"

#include "simpleTool.hpp"
#include "Structures.hpp"

#define ADC_CLOCK 4 // ns per ADC clock tick

class tracer : public simpleTool {
  private:
	unsigned int tlength;
 
	Trace *trace;

	TBranch *branch;
 	
	TH2I *hist;

	bool setAddresses();

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

tracer::tracer() : simpleTool(), tlength(0), trace(NULL), branch(NULL), hist(NULL) {
	max_entries_to_process = 1; 
	input_objname = "trace";
}

tracer::~tracer(){
	if(hist) delete hist;
}

void tracer::addOptions(){
}

bool tracer::processArgs(){
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

	unsigned int count = 0;
	while(getNextEntry()){
		// Skip empty traces.
		if(trace->wave.empty()) continue;

		// Get the trace length.
		if(count == 0){
			tlength = trace->wave.size()/trace->mult;
			std::cout << " Trace length is " << tlength << " ADC ticks (" << tlength*ADC_CLOCK << " ns).\n";
			hist = new TH2I("hist", "Traces", tlength, 0, tlength*ADC_CLOCK, max_entries_to_process, 0, max_entries_to_process);
			hist->SetStats(0);
		}
	
		int globalBin;
		for(unsigned int i = 0; i < tlength; i++){
			globalBin = hist->GetBin(i, count);
			hist->SetBinContent(globalBin, trace->wave.at(i));
		}
		count++;
	}

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
