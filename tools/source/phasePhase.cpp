#include <iostream>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"

#include "simpleTool.hpp"
#include "Structures.h"

class phasePhase : public simpleTool {
  private:
	int startID;
	int stopID;
	int padding;

	int startFile;
	int stopFile;

	int run;
	float p1, p2;
	float max1, max2;
	double tdiff;

	std::string input_prefix;

	void setFilename(int run);

	bool process();
	
  public:
	phasePhase() : simpleTool(), startID(0), stopID(1), padding(3), startFile(1), stopFile(1), input_prefix("") { }
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

void phasePhase::setFilename(int run){
	std::stringstream stream;
	stream << run;
	
	input_filename = input_prefix;
	if(stream.str().length() < (unsigned int)padding)
		input_filename += std::string(padding - stream.str().length(), '0');
	input_filename += stream.str() + ".root";
}

bool phasePhase::process(){
	if(!outtree) return false;

	std::cout << " Processing " << input_filename << ".\n";
	if(!openInputFile()) return false;

	if(!loadInputTree()){
		std::cout << "  Failed to load input tree!\n";
		return false;
	}

	TBranch *branch = NULL;
	TraceStructure *ptr = NULL;

	intree->SetBranchAddress("trace", &ptr, &branch);

	if(!branch){
		std::cout << " Error: Failed to load branch \"trace\" from input TTree.\n";
		return false;
	}

	std::cout << " Processing " << intree->GetEntries() << " entries.\n";
	for(int i = 0; i < intree->GetEntries(); i++){
		intree->GetEntry(i);
		if(ptr->mult == 0)
			continue;
		p1 = -1;
		p2 = -1;
		for(unsigned int j = 0; j < ptr->mult; j++){
			if(ptr->loc[j] == startID){
				p1 = ptr->phase[j]*4;
				max1 = ptr->maximum[j];
			}
			else if(ptr->loc[j] == stopID){
				p2 = ptr->phase[j]*4;
				max2 = ptr->maximum[j];
				tdiff = ptr->tdiff[j];
			}
		}
		if(p1 > 0 && p2 > 0)
			outtree->Fill();
	}

	return true;
}

void phasePhase::addOptions(){
	addOption(optionExt("start-id", required_argument, NULL, 'a', "<channel>", "Specify start detector ID or as a pair e.g. \"3,14\" (default is 0,0)."), userOpts, optstr);
	addOption(optionExt("stop-id", required_argument, NULL, 'b', "<channel>", "Specify stop detector ID or as a pair e.g. \"3,15\" (default is 0,1)."), userOpts, optstr);
	addOption(optionExt("files", required_argument, NULL, 'f', "<start:stop>", "Specify start and stop file as a pair e.g. \"1:32\" (default is 1:1)."), userOpts, optstr);
	addOption(optionExt("prefix", required_argument, NULL, 'p', "<prefix>", "Specify input filename prefix."), userOpts, optstr);
	addOption(optionExt("digits", required_argument, NULL, 'd', "<numZeros>", "Specify number of digits run number occupies e.g. 3 represents prefix_XXX.root (default is 3)."), userOpts, optstr);
}

bool phasePhase::processArgs(){
	if(userOpts.at(0).active){ // --start-id
		std::string userID = userOpts.at(0).argument;
		std::cout << userID << std::endl;
		if(userID.find(',') == std::string::npos)
			startID = strtol(userID.c_str(), NULL, 0);
		else{
			int mod = strtol(userID.substr(0, userID.find_first_of(',')).c_str(), NULL, 0);
			int chan = strtol(userID.substr(userID.find_first_of(',')+1).c_str(), NULL, 0);
			startID = mod*16 + chan;
		}
		std::cout << " Set start detector ID to " << startID << " (" << startID/16 << ", " << startID%16 << ").\n";
	}
	if(userOpts.at(1).active){ // --stop-id
		std::string userID = userOpts.at(1).argument;
		if(userID.find(',') == std::string::npos)
			stopID = strtol(userID.c_str(), NULL, 0);
		else{
			int mod = strtol(userID.substr(0, userID.find_first_of(',')).c_str(), NULL, 0);
			int chan = strtol(userID.substr(userID.find_first_of(',')+1).c_str(), NULL, 0);
			stopID = mod*16 + chan;
		}
		std::cout << " Set stop detector ID to " << stopID << " (" << stopID/16 << ", " << stopID%16 << ").\n";
	}
	if(userOpts.at(2).active){ // --file
		std::string userID = userOpts.at(2).argument;
		if(userID.find(':') != std::string::npos){
			startFile = strtol(userID.substr(0, userID.find_first_of(':')).c_str(), NULL, 0);
			stopFile = strtol(userID.substr(userID.find_first_of(':')+1).c_str(), NULL, 0);
		}
		else{
			std::cout << " Error: Invalid file range specification (" << userID << ")!\n";
			std::cout << "  File range must be given as pair delimited with ':' e.g. \"1:32\".\n";
			return false;
		}

		if(startFile < 1){
			std::cout << " Error: Invalid start run specification (" << startFile << ")!\n";
			return false;
		}
		if(stopFile < 1){
			std::cout << " Error: Invalid stop run specification (" << stopFile << ")!\n";
			return false;
		}
	}
	if(userOpts.at(3).active){ // --prefix
		input_prefix = userOpts.at(3).argument;
	}
	if(userOpts.at(4).active){ // --zeros
		padding = strtol(userOpts.at(4).argument.c_str(), NULL, 0);
		if(padding < 1){
			std::cout << " Error: Invalid zero padding specification (" << padding << ")!\n";
			return false;
		}
	}

	return true;
}

int phasePhase::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;	

	if(input_filename.empty() && input_prefix.empty()){
		std::cout << " Error: Must specify either input filename or input prefix!\n";
		return 1;
	}

	if(output_filename.empty()){
		std::cout << " Error: Output filename not specified!\n";
		return 2;
	}

	if(!openOutputFile()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 3;
	}
	
	outtree = new TTree("t", "tree");
	outtree->Branch("tdiff", &tdiff);
	outtree->Branch("p1", &p1);
	outtree->Branch("p2", &p2);
	outtree->Branch("max1", &max1);
	outtree->Branch("max2", &max2);
	outtree->Branch("run", &run);

	// Specifying a full filename takes precedence over filename prefix.
	if(!input_filename.empty()){
		process();
	}
	else{
		for(run = startFile; run <= stopFile; run++){
			setFilename(run);
			process();
		}
	}
	
	outfile->cd();
	outtree->Write();

	return 0;
}

int main(int argc, char *argv[]){
	phasePhase obj;
	
	return obj.execute(argc, argv);
}
