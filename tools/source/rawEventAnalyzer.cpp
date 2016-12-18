#include <stdlib.h>

#include "TBox.h"
#include "TLine.h"
#include "TMarker.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TCanvas.h"

#include "simpleTool.hpp"

std::vector<double> *chanTime;
std::vector<int> *chanID;
std::vector<bool> *inEvent;

double startEventTime;
double rawEventStartTime;
double rawEventStopTime;

TBranch *branches[6];

class rawEventAnalyzer : public simpleTool {
  private:
  	long long startEntry;
  	long long numEntries;

	TBox *box = NULL;
	TLine *line = NULL;
	TMarker *marker = NULL;

	bool setAddresses();

	void getEntry();

  public:
	rawEventAnalyzer();

	~rawEventAnalyzer();
	
	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);
};

bool rawEventAnalyzer::setAddresses(){
	if(!intree) return false;

	intree->SetBranchAddress("trig", &startEventTime, &branches[0]);
	intree->SetBranchAddress("start", &rawEventStartTime, &branches[1]);
	intree->SetBranchAddress("stop", &rawEventStopTime, &branches[2]);
	intree->SetBranchAddress("chanTime", &chanTime, &branches[3]);
	intree->SetBranchAddress("chanID", &chanID, &branches[4]);
	intree->SetBranchAddress("inEvent", &inEvent, &branches[5]);
	
	for(int i = 0; i < 6; i++){
		if(!branches[i]) return false;
	}
	
	return true;
}

void rawEventAnalyzer::getEntry(){
	if(!intree) return;

	std::vector<double> x, y;
	std::vector<int> colors;
	
	std::vector<double> trigTimes;
	std::vector<double> startTimes;
	std::vector<double> stopTimes;
	
	long long stopEntry = startEntry+numEntries;
	if(stopEntry > intree->GetEntries()) stopEntry = intree->GetEntries();
	
	int startCounts = 0;
	int goodCounts = 0;
	int badCounts = 0;
	
	int count = 0;
	for(long long entry = startEntry; entry < stopEntry; entry++){
		intree->GetEntry(entry);
	
		startTimes.push_back(rawEventStartTime);
		stopTimes.push_back(rawEventStopTime);
		
		if(startEventTime > 0) trigTimes.push_back(startEventTime);

		int currentColor;
		if(count++ % 2 == 0) currentColor = kBlue;
		else currentColor = kGreen+1;
		
		if(startEventTime > 0){
			y.push_back(0);
			x.push_back(startEventTime);
			colors.push_back(currentColor);
			startCounts++;
		}

		for(size_t i = 0; i < chanTime->size(); i++){
			y.push_back(chanID->at(i));
			x.push_back(chanTime->at(i));
			if(inEvent->at(i)){
				colors.push_back(currentColor);
				goodCounts++;
			}
			else{
				colors.push_back(kRed);
				badCounts++;
			}
		}
	}

	double earliestTime = 1E15;
	for(std::vector<double>::iterator iter = x.begin(); iter != x.end(); ++iter){
		if(*iter < earliestTime) earliestTime = *iter;
	}
	
	double tempTime;
	double minTime = 1E30;
	double maxTime = 0;
	std::vector<double>::iterator iterX, iterY;
	std::vector<int>::iterator iterC;
	
	for(iterX = x.begin(), iterY = y.begin(); iterX != x.end() && iterY != y.end(); ++iterX, ++iterY){
		tempTime = *iterX-earliestTime;
		if(tempTime < minTime) minTime = tempTime;
		if(tempTime > maxTime) maxTime = tempTime;
	}

	std::cout << startCounts << " start events, " << goodCounts << " in raw events, " << badCounts << " outside of raw event.\n";

	openCanvas1()->cd()->DrawFrame(minTime, 0, maxTime, 100);

	count = 0;
	for(iterX = startTimes.begin(), iterY = stopTimes.begin(); iterX != startTimes.end() && iterY != stopTimes.end(); ++iterX, ++iterY){
		if(count++ % 2 == 0){
			box->SetFillColor(kBlue);
			box->SetFillStyle(3004);
		}
		else{
			box->SetFillColor(kGreen+1);
			box->SetFillStyle(3005);
		}
		box->DrawBox(*iterX-earliestTime, 0, *iterY-earliestTime, 100);
	}

	for(iterX = trigTimes.begin(); iterX != trigTimes.end(); ++iterX){
		if(count++ % 2 == 0) line->SetLineColor(kBlue);
		else line->SetLineColor(kGreen+1);
		line->DrawLine(*iterX-earliestTime, 0, *iterX-earliestTime, 100);
	}
	
	count = 0;
	for(size_t index = 0; index < x.size(); index++){
		marker->SetMarkerColor(colors.at(index));
		marker->DrawMarker(x.at(index)-earliestTime, y.at(index));
	}
	
	can1->WaitPrimitive();
	
	if(!output_filename.empty()){
		outfile->cd();
		can1->Write("canvas");
		std::cout << " Saved TCanvas to file '" << output_filename << "'.\n";
	}
}

rawEventAnalyzer::rawEventAnalyzer() : simpleTool(), startEntry(0), numEntries(1) { 
	box = new TBox();
	box->SetFillColor(kBlue);
	
	line = new TLine();
	line->SetLineStyle(2);

	marker = new TMarker();
	marker->SetMarkerStyle(7);
	
	for(int i = 0; i < 6; i++) branches[i] = NULL;
	
	input_objname = "stats";
}

rawEventAnalyzer::~rawEventAnalyzer(){
	delete box;
	delete line;
	delete marker;
}

void rawEventAnalyzer::addOptions(){
	addOption(optionExt("start", required_argument, NULL, 's', "<start-entry>", "Specify the first tree entry."), userOpts, optstr);
	addOption(optionExt("num", required_argument, NULL, 'N', "<num-entries>", "Specify the number of entries to read."), userOpts, optstr);
}

bool rawEventAnalyzer::processArgs(){
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

int rawEventAnalyzer::execute(int argc, char *argv[]){
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
	
	return 0;
}

int main(int argc, char *argv[]){
	rawEventAnalyzer obj;
	
	return obj.execute(argc, argv);
}
