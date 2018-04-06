// helper.cpp
// Author: Cory R. Thornsberry
// Updated: May 18th, 2017

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

double convFactorX = 8E-9;
double convFactorY = 1;
std::ofstream ofile;

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

double setScale(const int &s){
	convFactorY = std::pow(10, 16-s)/8;
	std::cout << " Set integrator scale to (" << s << ")\n";
	std::cout << " Set y-axis conversion to (" << convFactorY << " enA/ns)\n";
	return convFactorY;
}

// Convert logic signal period to beam current (enA).
void conv(double &x, double &y){
	x *= convFactorX;
	y = convFactorY/y;
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
TGraph *convert(TGraph *g, const char *name="g"){
	TGraph *output = (TGraph*)(g->Clone(name));

	double x, y;
	for(int i = 0; i < g->GetN(); i++){
		g->GetPoint(i, x, y);
		conv(x, y);
		output->SetPoint(i, x, y);
	}

	return output;
}

// Convert a logic signal TGraph to a beam current vs. time graph.
TGraph *convert(TTree *t, const char *name="g"){
	t->Draw("tdiff:time");
	return convert((TGraph*)gPad->GetPrimitive("Graph"));
}

// Return the total integral of a logic signal TGraph.
double integrate(TGraph *g, double &sum, double &integral, double &stddev, unsigned int &N){
	double x1, y1;
	double x2, y2;
	g->GetPoint(0, x1, y1);
	sum += y1;
	for(int i = 1; i < g->GetN(); i++){
		g->GetPoint(i, x2, y2);
		sum += y2;
		integral += 0.5 * (y1 + y2) * (x2 - x1);
		x1 = x2;
		y1 = y2;
	}
	N += g->GetN();
	double mean = sum/N;
	for(int i = 0; i < g->GetN(); i++){
		g->GetPoint(i, x2, y2);
		stddev += (y2-mean)*(y2-mean);
	}
	std::cout << "mean = " << mean << " enA, stddev = " << std::sqrt(stddev/(g->GetN()-1)) << " enA, integral = " << integral << " nC\n";
	ofile << "\t" << mean << "\t" << std::sqrt(stddev/(g->GetN()-1)) << "\t" << integral << std::endl;
	return integral;
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
	
	return sum*convFactorX;
}

TGraph *process(TTree *t, double &sum, double &integral, double &stddev, unsigned int &N){
	TGraph *g = convert(t);
	integrate(g, sum, integral, stddev, N);
	return g;
}

void process(const char *fname, double &sum, double &integral, double &stddev, unsigned int &N){
	TGraph *graph = NULL;
	TFile *f = new TFile(fname, "READ");
	if(!f->IsOpen()) return NULL;
	TTree *t = (TTree*)f->Get("t");
	if(t){
		ofile << std::string(fname);
		graph = process(t, sum, integral, stddev, N);
	}

	f->Close();
	delete f;
	delete graph;
}

void process(const char *fname){
	double sum=0.0, integral=0.0, stddev=0.0;
	unsigned int N = 0;
	process(fname, sum, integral, stddev, N);
}


void help(const std::string &search_=""){
	// Colored terminal character string.
	const std::string dkred("\033[1;31m");
	const std::string dkblue("\033[1;34m");
	const std::string reset("\033[0m");

	// Defined global constants.
	//const std::string globalConstants[12] = {"const double", "Mn", "Mass of neutron, in MeV.",

	// Defined global variables.
	const std::string globalVariables[6] = {"double", "convFactorX", "Logic signal time conversion (default=8E-9)",
	                                        "double", "convFactorY", "Logic signal time-difference conversion"};

	// Defined functions.
	const std::string definedFunctions[36] = {"void", "conv", "double &x, double &y", "Convert logic signal period to beam current (enA).",
	                                          "TGraph*", "convert", "TGraph *g, const char *name=\"g\"", "Convert a logic signal TGraph to a beam current vs. time graph.",
	                                          "TGraph*", "convert", "TTree *t, const char *name=\"g\"", "Convert instantTime output TTree to beam current vs. time graph.",
	                                          "double", "integrate", "TGraph *g", "Return the total integral of a logic signal TGraph.",
	                                          "double", "mean", "TGraph *g", "Determine the mean beam current from a logic signal TGraph.",
	                                          "TGraph*", "process", "TTree *t", "Compute total logic signal integral from instantTime TTree output.",
	                                          "TGraph*", "process", "cnst char *fname", "Compute total logic signal integral from instantTime output file.",
	                                          "double", "setScale", "const double &s", "Set the integrator scale factor (default=8).",
	                                          "double", "summation", "TTree *t", "Return the total run time in seconds."};

	if(search_.empty()){
		std::cout << "*****************************\n";
		std::cout << "**          HELP           **\n";
		std::cout << "*****************************\n";
	
		/*std::cout << " Defined global constants:\n";
		for(int i = 0; i < 4; i++)
			std::cout << "  " << globalConstants[3*i+1] << std::endl;*/

		std::cout << "\n Defined global variables:\n";
		for(int i = 0; i < 2; i++)
			std::cout << "  " << globalVariables[3*i+1] << std::endl;

		std::cout << "\n Defined helper functions:\n";
		for(int i = 0; i < 9; i++)
			std::cout << "  " << definedFunctions[4*i+1] << std::endl;
			
		std::cout << std::endl;
	}
	else{
		size_t fIndex;
		std::string strings[3];
		
		/*for(int i = 0; i < 4; i++){
			fIndex = globalConstants[3*i+1].find(search_);
			if(fIndex != std::string::npos){
				strings[0] = globalConstants[3*i+1].substr(0, fIndex);
				strings[1] = globalConstants[3*i+1].substr(fIndex, search_.length());
				strings[2] = globalConstants[3*i+1].substr(fIndex+search_.length());
				std::cout << "  " << globalConstants[3*i];
				std::cout << " " << strings[0] << dkred << strings[1] << reset << strings[2];
				std::cout << dkblue << " //" << globalConstants[3*i+2] << reset << "\n";
			}
		}*/

		for(int i = 0; i < 2; i++){
			fIndex = globalVariables[3*i+1].find(search_);
			if(fIndex != std::string::npos){
				strings[0] = globalVariables[3*i+1].substr(0, fIndex);
				strings[1] = globalVariables[3*i+1].substr(fIndex, search_.length());
				strings[2] = globalVariables[3*i+1].substr(fIndex+search_.length());
				std::cout << "  " << globalVariables[3*i];
				std::cout << " " << strings[0] << dkred << strings[1] << reset << strings[2];
				std::cout << dkblue << " //" << globalVariables[3*i+2] << reset << "\n";
			}
		}
	
		for(int i = 0; i < 9; i++){
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

int logic(){
	ofile.open("logic.out");
	std::cout << " logic.cpp\n";
	std::cout << "  NOTE - type 'help()' to display a list of commands.\n";
	std::cout << "  NOTE - or 'help(string)' to search for a command or variable.\n";
	setScale(8);
	return 0;
}
