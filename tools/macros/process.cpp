// process.cpp
// Author: Cory R. Thornsberry
// Updated: May 18th, 2017

#include <stdlib.h>

class efficiencyFile;

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const double Mn = 10454.0750977429; // Mass of neutron, in MeV.
const double pi = 3.1415926536; // pi constant.
const double twopi = 6.2831853072; // 2*pi constant.

double d = 0.5; // Distance from target to detector, in m.
double dd = 0.03; // Thickness of detector, in m.
double dt = 1; // Timing resolution of detector, in ns.

double tofThreshold;
double energyThreshold;

efficiencyFile *effPtr;

///////////////////////////////////////////////////////////////////////////////
// Efficiency interpolation
///////////////////////////////////////////////////////////////////////////////

// Calculate neutron energy (MeV) given the time-of-flight (in ns).
double calcEnergy(const double &tof_){
	return (0.5*Mn*d*d/(tof_*tof_));
}

// Calculate neutron time-of-flight (ns) given the energy (in MeV).
double calcTOF(const double &E_){
	return (d*std::sqrt(Mn/(2*E_)));
}

class efficiencyFile{
  public:
	std::vector<double> E;
	std::vector<double> eff;

	efficiencyFile(){ }

	efficiencyFile(const char *fname){ load(fname); }

	bool empty(){ return E.empty(); }

	size_t size(){ return E.size(); }

	size_t load(const char *fname){
		std::ifstream ifile(fname);
		if(!ifile.good()) return 0;

		E.clear();
		eff.clear();

		std::string line;		
		double energy, efficiency;
		while(true){
			std::getline(ifile, line);
			if(ifile.eof()) break;

			if(line.empty() || line[0] == '#') continue;

			size_t splitIndex = line.find('\t');

			if(splitIndex == std::string::npos) continue;

			E.push_back(strtod(line.substr(0, splitIndex).c_str(), NULL));
			eff.push_back(strtod(line.substr(splitIndex+1).c_str(), NULL));

			//std::cout << "E=" << E.back() << ", eff=" << eff.back() << std::endl;
		}

		tofThreshold = calcTOF(E.front());
		energyThreshold = E.front();

		return E.size();
	}

	// Use linear interpolation to calculate an efficiency from the distribution.
	bool getEfficiency(const double &E_, double &efficiency){
		if(E_ > E.back()) return false;
		for(int i = 1; i < E.size(); i++){
			if(E_ >= E[i-1] && E_ < E[i]){
				efficiency = (eff[i-1] + (E_-E[i-1])*(eff[i]-eff[i-1])/(E[i]-E[i-1]));
				return true;
			}
		}
		return false;
	}

	// Scale an input function by the intrinsic efficiency of VANDLE at energy E_.
	double correctEfficiency(const double &E_, const double &funcVal_){
		double denom;
		if(this->getEfficiency(E_, denom))
			return funcVal_/denom;
		return 0.0;
	}

};

///////////////////////////////////////////////////////////////////////////////
// Neutron peak fit functions.
///////////////////////////////////////////////////////////////////////////////

class TOFfitFunctions{
  public:
	// Standard gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double gaussian(double *x, double *p){
		if(x[0] > tofThreshold) return 0.0;
		return effPtr->correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Gaus(x[0], p[1], p[2]));
	}

	// Logarithmic gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double logGaussian(double *x, double *p){
		if(x[0] > tofThreshold) return 0.0;
		return effPtr->correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Gaus(TMath::Log(x[0]), p[1], p[2]));
	}

	// Standard landau (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double landau(double *x, double *p){
		if(x[0] > tofThreshold) return 0.0;
		return effPtr->correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Landau(x[0], p[1], p[2]));
	}
	
	// Logarithmic landau (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double logLandau(double *x, double *p){
		if(x[0] > tofThreshold) return 0.0;
		return effPtr->correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Landau(TMath::Log(x[0]), p[1], p[2]));
	}	
};

class ENfitFunctions{
  public:
	// Standard gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double gaussian(double *x, double *p){
		if(x[0] < energyThreshold) return 0.0;
		return effPtr->correctEfficiency(x[0], p[0]*TMath::Gaus(x[0], p[1], p[2]));
	}

	// Logarithmic gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double logGaussian(double *x, double *p){
		if(x[0] < energyThreshold) return 0.0;
		return effPtr->correctEfficiency(x[0], p[0]*TMath::Gaus(TMath::Log(x[0]), p[1], p[2]));
	}

	// Standard landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double landau(double *x, double *p){
		if(x[0] < energyThreshold) return 0.0;
		return effPtr->correctEfficiency(x[0], p[0]*TMath::Landau(x[0], p[1], p[2]));
	}
	
	// Logarithmic landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double logLandau(double *x, double *p){
		if(x[0] < energyThreshold) return 0.0;
		return effPtr->correctEfficiency(x[0], p[0]*TMath::Landau(TMath::Log(x[0]), p[1], p[2]));
	}
};

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

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

// List the ratio of each bin in two 1-d histograms.
void binRatio(TH1 *h1_, TH1 *h2_){
	double content1, content2;
	double angleLow, angleHigh;
	double solidAngle;
	for(int i = 1; i <= h1_->GetNbinsX(); i++){
		content1 = h1_->GetBinContent(i);
		content2 = h2_->GetBinContent(i);
		angleLow = h1_->GetBinLowEdge(i);
		angleHigh = angleLow + h1_->GetBinWidth(i);
		solidAngle = twopi*(std::cos(angleLow*pi/180)-std::cos(angleHigh*pi/180));
		std::cout << i << "\t" << content1 << "\t" << content2 << "\t";
		std::cout << std::fixed << content2/content1 << "\t" << solidAngle << "\t" << solidAngle*(content2/content1) << std::endl;
		std::cout.unsetf(ios_base::floatfield);
	}
}

// Process a VANDMC monte carlo detector test output file.
bool processMCarlo(const char *fname, const int &comBins=18, const double &comLow=0, const double &comHigh=90){
	TFile *f = new TFile(fname, "READ");
	if(!f->IsOpen()) return false;
	
	TTree *t = (TTree*)f->Get("data");
	if(!t){
		f->Close();
		return false;
	}

	std::cout.precision(6);

	std::stringstream stream;
	stream << "comAngle>>(" << comBins << "," << comLow << "," << comHigh << ")";

	t->Draw(stream.str().c_str(), "", "");
	TH1 *h1 = (TH1*)t->GetHistogram()->Clone("h1");

	t->Draw(stream.str().c_str(), "mcarlo.mult>0", "");
	TH1 *h2 = (TH1*)t->GetHistogram()->Clone("h2");

	std::cout << "bin\tN1\tN2\tbinCoverage\tbinSolidAngle\tsolidAngle\n";

	binRatio(h1, h2);

	delete h1;
	delete h2;
	
	f->Close();
	delete f;
	
	return true;
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

// Process an output file from specFitter (simpleScan tool).
bool processSpecOutput(const char *fname, const char *ofname, const char *effname, bool energy_=false, const char *peakname="peak0"){
	TFile specFile(fname, "READ");
	if(!specFile.IsOpen()){
		std::cout << " Error! Failed to open input file \"" << fname << "\".\n";
		return false;
	}
	
	std::ofstream outFile(ofname);
	if(!outFile.good()){
		std::cout << " Error! Failed to open output file \"" << ofname << "\".\n";
		specFile.Close();
		return false;
	}

	efficiencyFile eff(effname);
	if(eff.empty()){
		std::cout << " Error! Failed to open efficiency file \"" << effname << "\".\n";
		specFile.Close();
		outFile.close();
		return false;
	}

	effPtr = &eff;

	// Get a list of the directories.
	TList *keyList = specFile.GetListOfKeys();
	int numKeys = specFile.GetNkeys();

	if(numKeys <= 6){
		std::cout << " Error! Failed to find peak data. Invalid data structure.\n";
		std::cout << "  Found the following keys...\n";
		for(int i = 0; i < numKeys; i++) std::cout << "  " << i << " - " << keyList->At(i)->GetName() << std::endl;
		specFile.Close();
		outFile.close();
	}

	outFile << "binID\tbinLow\tbinCenter\tbinErr\tchi2\tIbkg\thistCounts\tE\tEerr\tIpeak\tIcor\tIcorErr\tintrinseff\tbinSA\n";

	double En;
	double geomeff;
	double integral;

	int binID;

	double binLow;
	double binWidth;
	double chisquare;
	double funcNDF;

	double histCounts;
	double bkgCounts;
	double peakCounts;

	int functionType;
	bool gaussianFit = true;
	bool logXaxis = false;

	TF1 *peakfunc;

	std::string functionString;
	std::string currentDirectory;
	for(int i = 0; i < numKeys; i++){
		currentDirectory = std::string(keyList->At(i)->GetName());

		size_t index = currentDirectory.find("bin");

		// Check that this is not a directory.
		if(index == std::string::npos) continue;

		// Get the bin ID.
		binID = strtol(currentDirectory.substr(index+3).c_str(), NULL, 10);

		currentDirectory += "/";

		// Load the fit function from the file.
		peakfunc = (TF1*)getObject(&specFile, peakname, currentDirectory);

		if(!peakfunc){ // Backwards compatibility mode.
			if(strcmp(peakname, "peak0") == 0) 
				peakfunc = (TF1*)getObject(&specFile, "peakfunc", currentDirectory);
			else
				continue;
		}

		if(!peakfunc) continue;

		// Get the function string.
		functionString = (std::string)peakfunc->GetExpFormula();

		gaussianFit = (functionString.find("Gaus") != std::string::npos);
		logXaxis = (functionString.find("log(x)") != std::string::npos);

		// Select the proper function type.
		if(gaussianFit) functionType = (!logXaxis ? 0 : 2);
		else            functionType = (!logXaxis ? 1 : 3);

		getTNamed(&specFile, "binLow", binLow, currentDirectory);
		getTNamed(&specFile, "binWidth", binWidth, currentDirectory);
		getTNamed(&specFile, "chi2", chisquare, currentDirectory);
		getTNamed(&specFile, "NDF", funcNDF, currentDirectory);
		getTNamed(&specFile, "counts", histCounts, currentDirectory);
		getTNamed(&specFile, "Ibkg", bkgCounts, currentDirectory);

		outFile << binID << "\t" << binLow << "\t" << binLow+binWidth/2 << "\t" << binWidth/2 << "\t";
		outFile << chisquare/funcNDF << "\t" << bkgCounts << "\t" << histCounts << "\t";

		// Calculate the neutron energy.
		if(!energy_){
			if(functionType <= 1) En = 0.5*Mn*d*d/std::pow(peakfunc->GetParameter(1), 2.0);
			else                  En = 0.5*Mn*d*d/(TMath::Exp(2*(peakfunc->GetParameter(1)-std::pow(peakfunc->GetParameter(2), 2.0))));
			outFile << En << "\t" << En*std::sqrt(std::pow(dt/peakfunc->GetParameter(1), 2.0) + std::pow(dd/d, 2.0)) << "\t";
		}
		else{ 
			// Energy resolution in %.
			if(functionType <= 1) En = peakfunc->GetParameter(1);
			else                  En = TMath::Exp(peakfunc->GetParameter(1)-std::pow(peakfunc->GetParameter(2), 2.0));
			outFile << En << "\t" << 235.482*peakfunc->GetParameter(2)/peakfunc->GetParameter(1) << "\t";
		}
		
		// Load the projection histogram from the input file.
		TH1 *projhist = (TH1*)getObject(&specFile, "h1", currentDirectory);

		TF1 *peakIntrinsic;
		if(!energy_){
			if(functionType == 0) peakIntrinsic = new TF1("ff", TOFfitFunctions::gaussian, -10, 110, 3); // gaussian (function==0)
			else if(functionType == 1) peakIntrinsic = new TF1("ff", TOFfitFunctions::landau, -10, 110, 3); // landau (function==1)
			else if(functionType == 2) peakIntrinsic = new TF1("ff", TOFfitFunctions::logGaussian, 10, 110, 3); // logGaussian (function==2)
			else if(functionType == 3) peakIntrinsic = new TF1("ff", TOFfitFunctions::logLandau, 10, 110, 3); // logLandau (function==3)
		}
		else{
			if(functionType == 0) peakIntrinsic = new TF1("ff", ENfitFunctions::gaussian, 0, 10, 3); // gaussian (function==0)
			else if(functionType == 1) peakIntrinsic = new TF1("ff", ENfitFunctions::landau, 0, 10, 3); // landau (function==1)
			else if(functionType == 2) peakIntrinsic = new TF1("ff", ENfitFunctions::logGaussian, 0.1, 10, 3); // logGaussian (function==2)
			else if(functionType == 3) peakIntrinsic = new TF1("ff", ENfitFunctions::logLandau, 0.1, 10, 3); // logLandau (function==3)
		}

		// Calculate the number of neutrons.
		peakIntrinsic->SetParameter(0, peakfunc->GetParameter(0));
		peakIntrinsic->SetParameter(1, peakfunc->GetParameter(1));
		peakIntrinsic->SetParameter(2, peakfunc->GetParameter(2));
		integral = summation(projhist, peakIntrinsic);

		// Calculate the number of neutron counts (not efficiency corrected).
		peakCounts = summation(projhist, peakfunc);

		outFile << peakCounts << "\t" << integral << "\t0\t" << peakCounts/integral << "\t";
		outFile << (twopi*(-std::cos((binLow+binWidth)*pi/180) + std::cos(binLow*pi/180))) << std::endl;

		delete peakIntrinsic;
		
	}

	specFile.Close();
	outFile.close();
	
	return true;
}

void help(const std::string &search_=""){
	// Colored terminal character string.
	const std::string dkred("\033[1;31m");
	const std::string dkblue("\033[1;34m");
	const std::string reset("\033[0m");

	// Defined global constants.
	const std::string globalConstants[9] = {"const double", "Mn", "Mass of neutron, in MeV.",
	                                         "const double", "pi", "pi constant.",
	                                         "const double", "twopi", "2*pi constant."};

	// Defined global variables.
	const std::string globalVariables[9] = {"double", "d", "Distance from target to detector, in m.",
	                                        "double", "dd", "Thickness of detector, in m.",
	                                        "double", "dt", "Timing resolution of detector, in ns."};

	// Defined functions.
	const std::string definedFunctions[44] = {"void", "binRatio", "TH1 *h1_, TH1 *h2_", "List the ratio of each bin in two 1-d histograms.",
	                                          "double", "calcEnergy", "const double &tof_", "Calculate neutron energy (MeV) given the time-of-flight (in ns).",
	                                          "double", "calcTOF", "const double &E_", "Calculate neutron time-of-flight (ns) given the energy (in MeV).",
	                                          "double", "TOFfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "TOFfitFunctions::landau", "double *x, double *p", "Standard landau (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "ENfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "ENfitFunctions::landau", "double *x, double *p", "Standard landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "interpolate", "const double *py, const double &E, double &eff", "Use linear interpolation to calculate a value from a distribution.",
	                                          "void", "processMCarlo", "const char *fname, const int &comBins=18, const double &comLow=0, const double &comHigh=90", "Process a VANDMC monte carlo detector test output file.",
	                                          "bool", "processSpecOutput", "const char *fname, const char *ofname, const char *effname, bool energy_=false", "Process an output file from specFitter (simpleScan tool).",
	                                          "double", "summation", "TH1 *h, TF1 *f", "Return the total number of counts under a TF1."};

	if(search_.empty()){
		std::cout << "*****************************\n";
		std::cout << "**          HELP           **\n";
		std::cout << "*****************************\n";
	
		std::cout << " Defined global constants:\n";
		for(int i = 0; i < 3; i++)
			std::cout << "  " << globalConstants[3*i+1] << std::endl;
	
		std::cout << "\n Defined global variables:\n";
		for(int i = 0; i < 3; i++)
			std::cout << "  " << globalVariables[3*i+1] << std::endl;
	
		std::cout << "\n Defined helper functions:\n";
		for(int i = 0; i < 11; i++)
			std::cout << "  " << definedFunctions[4*i+1] << std::endl;
			
		std::cout << std::endl;
	}
	else{
		size_t fIndex;
		std::string strings[3];
		
		for(int i = 0; i < 3; i++){
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
	
		for(int i = 0; i < 3; i++){
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
	
		for(int i = 0; i < 11; i++){
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

int process(){
	std::cout << " process.cpp\n";
	std::cout << "  NOTE - type 'help()' to display a list of commands.\n";
	std::cout << "  NOTE - or 'help(string)' to search for a command or variable.\n";
	return 0;
}
