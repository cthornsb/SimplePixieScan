// process.cpp
// Author: Cory R. Thornsberry
// Updated: May 18th, 2017

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const double Mn = 10454.0750977429; // Mass of neutron, in MeV.
const double pi = 3.1415926536; // pi constant.
const double twopi = 6.2831853072; // 2*pi constant.

double d = 0.5; // Distance from target to detector, in m.
double dd = 0.03; // Thickness of detector, in m.
double dt = 1; // Timing resolution of detector, in ns.

///////////////////////////////////////////////////////////////////////////////
// Efficiency interpolation
///////////////////////////////////////////////////////////////////////////////

// Array of energies for use in efficiency interpolation.
const double effEnergy[44] = {-9999, 0.0250, 0.0450, 0.0650, 0.0850, 0.1050, 0.1250, 0.1500, 0.1800, 0.2100, 
                               0.2400, 0.2800, 0.3300, 0.3800, 0.4300, 0.4800, 0.5300, 0.5800, 0.6300, 0.6800, 
                               0.7300, 0.7800, 0.8300, 0.8800, 0.9300, 1.0025, 1.1000, 1.2000, 1.3000, 1.4000, 
                               1.5000, 1.6000, 1.7000, 1.8000, 1.9000, 2.0500, 2.2500, 2.4500, 2.6500, 2.8500, 
                               3.1000, 3.4000, 3.7000, 4.0000};

// Array of zero-threshold efficiencies for use in efficiency interpolation.
const double effZero[44] = {0.045, -4.7827, 0.5460, 0.7381, 0.7755, 0.8057, 0.9023, 0.8351, 0.7991, 0.7651, 
                            0.7505, 0.7014, 0.6514, 0.6123, 0.5737, 0.5557, 0.5286, 0.5202, 0.4757, 0.4606, 
                            0.4447, 0.4320, 0.4395, 0.4221, 0.4345, 0.4132, 0.3904, 0.3918, 0.3800, 0.3746, 
                            0.3658, 0.3636, 0.3591, 0.3485, 0.3534, 0.3524, 0.3433, 0.3408, 0.3320, 0.3285, 
                            0.3147, 0.3185, 0.3190, 0.2514};

// Array of Barium threshold efficiencies for use in efficiency interpolation.
const double effBa133[44] = {0.15, -7.751, -1.251, -0.540, -0.243, -0.155, -0.015, 0.001, 0.033, 0.077, 
                             0.143, 0.223, 0.312, 0.358, 0.371, 0.379, 0.380, 0.380, 0.371, 0.370, 
                             0.362, 0.353, 0.343, 0.328, 0.344, 0.326, 0.311, 0.311, 0.303, 0.300, 
                             0.291, 0.289, 0.284, 0.275, 0.282, 0.261, 0.265, 0.263, 0.250, 0.242, 
                             0.230, 0.221, 0.218, 0.174};

// Array of Americium threshold efficiencies for use in efficiency interpolation.
const double effAm241[44] = {0.24, -7.590, -1.508, -0.653, -0.358, -0.223, -0.085, -0.060, -0.025, -0.004, 
                             0.006, 0.007, 0.007, 0.024, 0.064, 0.119, 0.170, 0.214, 0.235, 0.262, 
                             0.266, 0.267, 0.267, 0.263, 0.281, 0.267, 0.263, 0.266, 0.262, 0.263, 
                             0.259, 0.261, 0.256, 0.248, 0.256, 0.253, 0.242, 0.243, 0.232, 0.224, 
                             0.213, 0.204, 0.198, 0.158};

const double *effPtr = effAm241;
const double *enPtr = effEnergy;
int nPts = 44;

// Calculate neutron energy (MeV) given the time-of-flight (in ns).
double calcEnergy(const double &tof_){
	return (0.5*Mn*d*d/(tof_*tof_));
}

// Calculate neutron time-of-flight (ns) given the energy (in MeV).
double calcTOF(const double &E_){
	return (d*std::sqrt(Mn/(2*E_)));
}

// Use linear interpolation to calculate a value from a distribution.
bool interpolate(const double *py, const double &E, double &eff){
	if(E < py[0]) return false;
	if(E < enPtr[nPts-1]){ // Interpolate.
		for(int i = 2; i < nPts; i++){
			if(E >= enPtr[i-1] && E < enPtr[i]){
				eff = py[i-1] + (E-enPtr[i-1])*(py[i]-py[i-1])/(enPtr[i]-enPtr[i-1]);
				return true;
			}
		}
	}
	else{
		// Extrapolate.
		const double p0 = 0.299687, p1 = -1.67919, p2 = -0.665115;
		eff = p0 + std::exp(p1 + p2*E);
	}
	return true;
}

// Scale an input function by the intrinsic efficiency of VANDLE at energy E_.
double correctEfficiency(const double &E_, const double &funcVal_){
	double denom;
	if(interpolate(effPtr, E_, denom))
		return funcVal_/denom;
	return 0.0;
}

///////////////////////////////////////////////////////////////////////////////
// Neutron peak fit functions.
///////////////////////////////////////////////////////////////////////////////

class TOFfitFunctions{
  public:
	// Standard gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double gaussian(double *x, double *p){
		if(x[0] > 105) return 0.0;
		return correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Gaus(x[0], p[1], p[2]));
	}

	// Logarithmic gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double logGaussian(double *x, double *p){
		if(x[0] > 105) return 0.0;
		return correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Gaus(TMath::Log(x[0]), p[1], p[2]));
	}

	// Standard landau (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double landau(double *x, double *p){
		if(x[0] > 105) return 0.0;
		return correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Landau(x[0], p[1], p[2]));
	}
	
	// Logarithmic landau (x in ns) scaled by linearly interpolated intrinsic efficiency. 
	static double logLandau(double *x, double *p){
		if(x[0] > 105) return 0.0;
		return correctEfficiency(calcEnergy(x[0]), p[0]*TMath::Landau(TMath::Log(x[0]), p[1], p[2]));
	}	
};

class ENfitFunctions{
  public:
	// Standard gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double gaussian(double *x, double *p){
		if(x[0] < 0.125) return 0.0;
		return correctEfficiency(x[0], p[0]*TMath::Gaus(x[0], p[1], p[2]));
	}

	// Logarithmic gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double logGaussian(double *x, double *p){
		if(x[0] < 0.125) return 0.0;
		return correctEfficiency(x[0], p[0]*TMath::Gaus(TMath::Log(x[0]), p[1], p[2]));
	}

	// Standard landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double landau(double *x, double *p){
		if(x[0] < 0.125) return 0.0;
		return correctEfficiency(x[0], p[0]*TMath::Landau(x[0], p[1], p[2]));
	}
	
	// Logarithmic landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.
	static double logLandau(double *x, double *p){
		if(x[0] < 0.125) return 0.0;
		return correctEfficiency(x[0], p[0]*TMath::Landau(TMath::Log(x[0]), p[1], p[2]));
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
	for(int i = 1; i <= h1_->GetNbinsX(); i++){
		content1 = h1_->GetBinContent(i);
		content2 = h2_->GetBinContent(i);
		std::cout << i << "\t" << content1 << "\t" << content2 << "\t" << content2/content1 << std::endl;
	}
}

// Process a VANDMC monte carlo detector test output file.
bool processMCarlo(const char *fname, const char *tname="data"){
	const int comBins = 18;
	const double comLow = 0;
	const double comHigh = 90;

	TFile *f = new TFile(fname, "READ");
	if(!f->IsOpen()) return false;
	
	TTree *t = (TTree*)f->Get(tname);
	if(!t){
		f->Close();
		return false;
	}

	std::stringstream stream;
	stream << "comAngle>>(" << comBins << "," << comLow << "," << comHigh << ")";

	t->Draw(stream.str().c_str(), "", "");
	TH1 *h1 = (TH1*)t->GetHistogram()->Clone("h1");

	t->Draw(stream.str().c_str(), "mcarlo.mult>0", "");
	TH1 *h2 = (TH1*)t->GetHistogram()->Clone("h2");

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
bool processSpecOutput(const char *fname, const char *ofname, const double *ptr_=effZero, bool energy_=false){
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

	effPtr = ptr_;

	outFile << "binID\tbinLow\tbinCenter\tbinErr\tchi2\tIbkg\thistCounts\tp0\tp0err\tp1\tp1err\tp2\tp2err\tA\tAerr\tmu\tmuerr\tE\tEerr\tsigma\tsigmaerr\tIpeak\tIcor\tIcorErr\tintrinseff\tbinSA\n";

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

	double peakPars[12];

	int functionType;
	bool gaussianFit = true;
	bool logXaxis = false;

	TF1 *func;

	std::string functionString;
	std::string currentDirectory;
	for(int i = 6; i < numKeys; i++){
		currentDirectory = std::string(keyList->At(i)->GetName());

		// Get the bin ID.
		binID = strtol(currentDirectory.substr(currentDirectory.find("bin")+3).c_str(), NULL, 10);

		currentDirectory += "/";

		// Load the fit function from the file.
		func = (TF1*)getObject(&specFile, "func", currentDirectory);

		if(!func) continue;

		// Get the function string.
		functionString = (std::string)func->GetExpFormula();

		gaussianFit = (functionString.find("Gaus") != std::string::npos);
		logXaxis = (functionString.find("log(x)") != std::string::npos);

		// Select the proper function type.
		if(gaussianFit) functionType = (!logXaxis ? 0 : 2);
		else            functionType = (!logXaxis ? 1 : 3);

		// Get the 6 fit parameters.
		for(int j = 0; j < 6; j++){
			peakPars[2*j] = func->GetParameter(j);
			peakPars[2*j+1] = func->GetParError(j);
		}

		getTNamed(&specFile, "binLow", binLow, currentDirectory);
		getTNamed(&specFile, "binWidth", binWidth, currentDirectory);
		getTNamed(&specFile, "chi2", chisquare, currentDirectory);
		getTNamed(&specFile, "NDF", funcNDF, currentDirectory);
		getTNamed(&specFile, "counts", histCounts, currentDirectory);
		getTNamed(&specFile, "Ibkg", bkgCounts, currentDirectory);
		getTNamed(&specFile, "Ipeak", peakCounts, currentDirectory);

		outFile << binID << "\t" << binLow << "\t" << binLow+binWidth/2 << "\t" << binWidth/2 << "\t";
		outFile << chisquare/funcNDF << "\t" << bkgCounts << "\t" << histCounts << "\t";
		for(int j = 0; j <= 4; j++){
			outFile << peakPars[2*j] << "\t" << peakPars[2*j+1] << "\t";
		}

		// Calculate the neutron energy.
		if(!energy_){
			if(functionType <= 1) En = 0.5*Mn*d*d/(peakPars[8]*peakPars[8]);
			else                  En = 0.5*Mn*d*d/(TMath::Exp(2*(peakPars[8]-peakPars[10]*peakPars[10])));
			outFile << En << "\t" << En*std::sqrt(std::pow(dt/peakPars[8], 2.0) + std::pow(dd/d, 2.0)) << "\t";
		}
		else{ 
			// Energy resolution in %.
			if(functionType <= 1) En = peakPars[8];
			else                  En = TMath::Exp(peakPars[8]-peakPars[10]*peakPars[10]);
			outFile << En << "\t" << 235.482*peakPars[10]/peakPars[8] << "\t";
		}
	
		outFile << peakPars[10] << "\t" << peakPars[11] << "\t" << peakCounts << "\t";
	
		TF1 *peakfunc = (TF1*)getObject(&specFile, "peakfunc", currentDirectory);
		TH1 *projhist = (TH1*)getObject(&specFile, "h1", currentDirectory);

		if(!energy_){
			if(functionType == 0) peakfunc = new TF1("ff", TOFfitFunctions::gaussian, -10, 110, 3); // gaussian (function==0)
			else if(functionType == 1) peakfunc = new TF1("ff", TOFfitFunctions::landau, -10, 110, 3); // landau (function==1)
			else if(functionType == 2) peakfunc = new TF1("ff", TOFfitFunctions::logGaussian, 10, 110, 3); // logGaussian (function==2)
			else if(functionType == 3) peakfunc = new TF1("ff", TOFfitFunctions::logLandau, 10, 110, 3); // logLandau (function==3)
		}
		else{
			if(functionType == 0) peakfunc = new TF1("ff", ENfitFunctions::gaussian, 0, 10, 3); // gaussian (function==0)
			else if(functionType == 1) peakfunc = new TF1("ff", ENfitFunctions::landau, 0, 10, 3); // landau (function==1)
			else if(functionType == 2) peakfunc = new TF1("ff", ENfitFunctions::logGaussian, 0.1, 10, 3); // logGaussian (function==2)
			else if(functionType == 3) peakfunc = new TF1("ff", ENfitFunctions::logLandau, 0.1, 10, 3); // logLandau (function==3)
		}
	
		double pars[3];

		double intMax = -1E30;
		double intMin = 1E30;

		const int val1[8] = {-1, -1, -1, 1, -1, 1, 1, 1};
		const int val2[8] = {-1, -1, 1, -1, 1, 1, -1, 1};
		const int val3[8] = {-1, 1, -1, -1, 1, -1, 1, 1};

		for(unsigned int i = 0; i < 8; i++){
			peakfunc->SetParameter(0, peakPars[6]+val1[i]*peakPars[7]);
			peakfunc->SetParameter(1, peakPars[8]+val1[i]*peakPars[9]);
			peakfunc->SetParameter(2, peakPars[10]+val1[i]*peakPars[11]);

			integral = summation(projhist, peakfunc);

			if(integral < intMin) intMin = integral;
			if(integral > intMax) intMax = integral;
		}

		// Calculate the number of neutrons.
		peakfunc->SetParameter(0, peakPars[6]);
		peakfunc->SetParameter(1, peakPars[8]);
		peakfunc->SetParameter(2, peakPars[10]);
		integral = summation(projhist, peakfunc);

		if((intMax - integral) >= (integral - intMin))
			outFile << integral << "\t" << intMax-integral << "\t" << peakCounts/integral << "\t";
		else
			outFile << integral << "\t" << integral-intMin << "\t" << peakCounts/integral << "\t";
		
		outFile << (twopi*(-std::cos((binLow+binWidth)*pi/180) + std::cos(binLow*pi/180))) << std::endl;

		delete peakfunc;
		
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
	const std::string globalConstants[21] = {"const double", "Mn", "Mass of neutron, in MeV.",
	                                         "const double", "pi", "pi constant.",
	                                         "const double", "twopi", "2*pi constant.",
	                                         "const double", "effEnergy[44]", "Array of energies for use in efficiency interpolation.",
	                                         "const double", "effZero[44]", "Array of hardware threshold efficiencies for use in efficiency interpolation.",
	                                         "const double", "effBa133[44]", "Array of 133Ba threshold efficiencies for use in efficiency interpolation.",
	                                         "const double", "effAm241[44]", "Array of 241Am threshold efficiencies for use in efficiency interpolation."};

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
	                                          "void", "processMCarlo", "const char *fname, const char *tname=\"data\"", "Process a VANDMC monte carlo detector test output file.",
	                                          "bool", "processSpecOutput", "const char *fname, const char *ofname, const double *ptr_=effZero, bool energy_=false", "Process an output file from specFitter (simpleScan tool).",
	                                          "double", "summation", "TH1 *h, TF1 *f", "Return the total number of counts under a TF1."};

	if(search_.empty()){
		std::cout << "*****************************\n";
		std::cout << "**          HELP           **\n";
		std::cout << "*****************************\n";
	
		std::cout << " Defined global constants:\n";
		for(int i = 0; i < 7; i++)
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
		
		for(int i = 0; i < 7; i++){
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
