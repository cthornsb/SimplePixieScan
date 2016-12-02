#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

#include "TFile.h"
#include "TTree.h"
#include "TObject.h"
#include "TCollection.h" // TIter class

#include "simpleTool.hpp"

class mapReader : public simpleTool {
  private:
	const int padding = 2;

	const std::string pathPrefix = "/map/mod";

	const std::string chanNames[16] = {"chan00", "chan01", "chan02", "chan03", "chan04", "chan05", "chan06", "chan07",
                                           "chan08", "chan09", "chan10", "chan11", "chan12", "chan13", "chan14", "chan15"};

	int startMod;
	int stopMod;

	void setName(int run);

	bool process(int mod, std::ofstream *file_);

  public:
	mapReader() : simpleTool(), startMod(0), stopMod(12) { }
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

bool mapReader::process(int mod, std::ofstream *file_){
	if(!file_) return false;

	std::stringstream stream;
	stream << mod;
	
	// Get the root path of the current module's map.
	std::string currentName = pathPrefix;
	if(stream.str().length() < (unsigned int)padding)
		currentName += std::string(padding - stream.str().length(), '0');
	currentName += stream.str() + "/";

	// Load the currect directory.
	TDirectory *dir = infile->GetDirectory(currentName.c_str());
	
	// Failed to load directory.
	if(!dir) return false;

	// Get list of map entries.
	TIter iter(dir->GetListOfKeys());

	bool output = false;
	while(true){
		TObject *obj = iter();
		if(!obj) break;

		// Write the map entry to the file.
		(*file_) << obj->GetName() << std::endl;

		output = true;
	}

	return output;
}

void mapReader::addOptions(){
	addOption(optionExt("modules", required_argument, NULL, 'm', "<start:stop>", "Specify start and stop module as a pair e.g. \"3:7\" (default is 0:12)."), userOpts, optstr);
}

bool mapReader::processArgs(){
	if(userOpts.at(0).active){ // --modules
		std::string userID = userOpts.at(0).argument;
		if(userID.find(':') != std::string::npos){
			startMod = strtol(userID.substr(0, userID.find_first_of(':')).c_str(), NULL, 0);
			stopMod = strtol(userID.substr(userID.find_first_of(':')+1).c_str(), NULL, 0);
		}
		else{
			std::cout << " Error: Invalid module range specification (" << userID << ")!\n";
			std::cout << "  Module range must be given as pair delimited with ':' e.g. \"3:7\".\n";
			return false;
		}

		if(startMod < 0){
			std::cout << " Error: Invalid start module specification (" << startMod << ")!\n";
			return false;
		}
		if(stopMod < 0){
			std::cout << " Error: Invalid stop module specification (" << stopMod << ")!\n";
			return false;
		}
	}

	return true;
}

int mapReader::execute(int argc, char *argv[]){
	if(!setup(argc, argv))
		return 0;	

	if(input_filename.empty()){
		std::cout << " Error: Input filename not specified!\n";
		return 1;
	}

	if(output_filename.empty())
		output_filename = "map.dat";

	if(!openInputFile()){
		std::cout << " Error: Failed to load input file \"" << input_filename << "\".\n";
		return 2;
	}

	std::ofstream ofile(output_filename.c_str());
	if(!ofile.good()){
		std::cout << " Error: Failed to load output file \"" << output_filename << "\".\n";
		return 3;
	}

	// Get the current time.
	time_t now = time(0);

	// Append a header to the output map file.
	ofile << "# SimplePixieScan map file\n";
	ofile << "# Generated by mapReader on " << ctime(&now);
	ofile << "# Input root file: " << input_filename << std::endl;

	int mod;
	for(mod = startMod; mod <= stopMod; mod++){
		if(process(mod, &ofile)) std::cout << " Read entries from module " << mod << std::endl;
	}

	ofile.close();

	return 0;
}

int main(int argc, char *argv[]){
	mapReader obj;
	
	return obj.execute(argc, argv);
}
