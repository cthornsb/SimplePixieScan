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
	std::vector<double> deff;

	efficiencyFile(){ }

	efficiencyFile(const char *fname){ load(fname); }

	bool empty(){ return E.empty(); }

	size_t size(){ return E.size(); }

	size_t load(const char *fname){
		std::ifstream ifile(fname);
		if(!ifile.good()) return 0;

		E.clear();
		eff.clear();
		deff.clear();

		std::string line;		
		double energy, efficiency;
		while(true){
			std::getline(ifile, line);
			if(ifile.eof()) break;

			if(line.empty() || line[0] == '#') continue;

			size_t splitIndex1 = line.find('\t');

			if(splitIndex1 == std::string::npos) continue;

			// Read the energy.			
			E.push_back(strtod(line.substr(0, splitIndex1).c_str(), NULL));

			// Read the efficiency and the uncertainty (if available).
			size_t splitIndex2 = line.find('\t', splitIndex1+1);
			if(splitIndex2 != std::string::npos){
				eff.push_back(strtod(line.substr(splitIndex1+1, splitIndex2).c_str(), NULL));
				deff.push_back(strtod(line.substr(splitIndex2+1).c_str(), NULL));
			}
			else{
				eff.push_back(strtod(line.substr(splitIndex1+1).c_str(), NULL));
				deff.push_back(0);
			}

			//std::cout << " debug: E=" << E.back() << ", eff=" << eff.back() << ", deff=" << deff.back() << std::endl;
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

	// Use linear interpolation to calculate an efficiency from the distribution.
	bool getEfficiency(const double &E_, double &efficiency, double &uncertainty){
		if(E_ > E.back()) return false;
		for(int i = 1; i < E.size(); i++){
			if(E_ >= E[i-1] && E_ < E[i]){
				efficiency = (eff[i-1] + (E_-E[i-1])*(eff[i]-eff[i-1])/(E[i]-E[i-1]));
				uncertainty = (deff[i-1]+deff[i])/2;
				return true;
			}
		}
		return false;
	}

	// Approximate the uncertainty in a given efficiency by averaging the two neighboring points.
	bool getUncertainty(const double &Elow_, const double &efficiency, double &uncertainty){
		for(int i = 1; i < E.size(); i++){
			if(E[i-1] > Elow_ && ((efficiency >= eff[i-1] && efficiency < eff[i]) || (efficiency >= eff[i] && efficiency < eff[i-1]))){
				uncertainty = (deff[i-1]+deff[i])/2;
			}
		}
		return false;
	}

	// Scale an input function by the intrinsic efficiency of VANDLE at energy E_.
	double correctEfficiency(const double &E_, const double &funcVal_){
		double denom = 0;
		if(this->getEfficiency(E_, denom) && denom != 0)
			return funcVal_/denom;
		return 0.0;
	}

	// Return a new TGraphErrors containing all the data points.
	TGraphErrors *graph(){
		TGraphErrors *output = new TGraphErrors(E.size());
		for(size_t i = 0; i < E.size(); i++){
			output->SetPoint(i, E[i], eff[i]);
			output->SetPointError(i, 0, deff[i]);
		}
		return output;
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
void binRatio(TH1 *h1_, TH1 *h2_, std::ostream &out=std::cout){
	out << "bin\tN1\tN2\tbinCoverage\tbinSolidAngle\tsolidAngle\n";
	out.precision(6);
	double content1, content2;
	double angleLow, angleHigh;
	double solidAngle;
	for(int i = 1; i <= h1_->GetNbinsX(); i++){
		content1 = h1_->GetBinContent(i);
		content2 = h2_->GetBinContent(i);
		angleLow = h1_->GetBinLowEdge(i);
		angleHigh = angleLow + h1_->GetBinWidth(i);
		solidAngle = twopi*(std::cos(angleLow*pi/180)-std::cos(angleHigh*pi/180));
		out << i << "\t" << content1 << "\t" << content2 << "\t";
		out << std::fixed << content2/content1 << "\t" << solidAngle << "\t" << solidAngle*(content2/content1) << std::endl;
		out.unsetf(ios_base::floatfield);
	}
}

// List the ratio of each bin in two 1-d histograms.
void binRatio(TH1 *h1_, TH1 *h2_, const std::vector<double> &comAngles_, std::ostream &out=std::cout){
	out << "\nbin\tlabCenter\tcomLow\tcomCenter\tcomErr\tN1\tN2\tbinSolidAngle\tsolidAngle\n";
	out.precision(6);
	double binWidth;
	double content1, content2;
	double angleLow, angleHigh;
	double solidAngle;
	for(int i = 1; i <= h1_->GetNbinsX(); i++){
		content1 = h1_->GetBinContent(i);
		content2 = h2_->GetBinContent(i);

		angleLow = comAngles_.at(i-1);
		angleHigh = comAngles_.at(i);

		if(angleLow > angleHigh){ // Inverse kinematics.
			double tempValue = angleLow;
			angleLow = angleHigh;
			angleHigh = tempValue;
		}

		binWidth = angleHigh-angleLow;
		
		solidAngle = twopi*(std::cos(angleLow*pi/180)-std::cos(angleHigh*pi/180));
		out << i << "\t" << h1_->GetBinCenter(i) << "\t" << angleLow << "\t" << angleLow+binWidth/2 << "\t" << binWidth/2 << "\t" << content1 << "\t" << content2 << "\t";
		out << std::fixed << solidAngle << "\t" << solidAngle*(content2/content1) << std::endl;
		out.unsetf(ios_base::floatfield);
	}
}

// Process a VANDMC monte carlo detector test output file.
bool processMCarlo(const char *fname, std::ostream &out=std::cout, const int &comBins=18, const double &comLow=0, const double &comHigh=90){
	TFile *f = new TFile(fname, "READ");
	if(!f->IsOpen()) return false;
	
	TTree *t = (TTree*)f->Get("data");
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

	binRatio(h1, h2, out);

	delete h1;
	delete h2;
	
	f->Close();
	delete f;
	
	return true;
}

// Process a VANDMC monte carlo detector test output file.
bool processMCarlo(const char *fname, const char *binfname){
	TFile *f = new TFile(fname, "READ");
	if(!f->IsOpen()) return false;
	
	TTree *t = (TTree*)f->Get("data");
	if(!t){
		f->Close();
		return false;
	}

	std::ifstream binfile(binfname);
	if(!binfile.good()){
		f->Close();
		return false;
	}

	std::vector<double> xbinsLab;
	std::vector<double> xbinsCom;

	std::cout << "\nbin\tlowCom\tlowLab\n";

	int count = 1;
	double tempLabValue, tempComValue;
	while(true){
		binfile >> tempLabValue >> tempComValue;
		if(binfile.eof()) break;
		xbinsLab.push_back(tempLabValue);
		xbinsCom.push_back(tempComValue);
		std::cout << count++ << "\t" << tempComValue << "\t" << tempLabValue << std::endl;
	}

	binfile.close();

	TH1F *h1 = new TH1F("h1", "h1", xbinsLab.size()-1, xbinsLab.data());
	TH1F *h2 = new TH1F("h2", "h2", xbinsLab.size()-1, xbinsLab.data());
	
	t->Draw("labTheta>>h1", "", "");
	t->Draw("labTheta>>h2", "mult>0", "");

	h1->Draw();
	h2->SetLineColor(kRed);
	h2->Draw("SAME");

	gPad->WaitPrimitive();
	gPad->Close();

	binRatio(h1, h2, xbinsCom);

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
bool processSpecOutput(const char *fname, const char *ofname, const char *effname, bool energy_=false, int peak=0){
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

	outFile << "binID\tbinLow\tbinCenter\tbinErr\tchi2\tIbkg\thistCounts\tE\tEerr\tIpeak\tNdet\tdNdet\tintrinsic\tdintrinsic\n";

	double En;
	double geomeff;
	double Ndet;
	double dNdet;
	double intrinsic;
	double dintrinsic;

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
	TFitResult *fitresult;

	std::stringstream stream;
	stream << "peak" << peak;
	std::string peakname = stream.str();

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
		peakfunc = (TF1*)getObject(&specFile, peakname.c_str(), currentDirectory);

		if(!peakfunc){ // Backwards compatibility mode.
			if(peak == 0) 
				peakfunc = (TF1*)getObject(&specFile, "peakfunc", currentDirectory);
			else
				peakfunc = NULL;
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

		// Calculate the number of neutron counts (not efficiency corrected).
		peakCounts = summation(projhist, peakfunc);

		// Calculate the number of neutrons.
		peakIntrinsic->SetParameter(0, peakfunc->GetParameter(0));
		peakIntrinsic->SetParameter(1, peakfunc->GetParameter(1));
		peakIntrinsic->SetParameter(2, peakfunc->GetParameter(2));
		Ndet = summation(projhist, peakIntrinsic);

		// Compute uncertainty in Ndet.
		int ampPar, sigPar;
		int NbkgPar = 0;
		TF1 *bkgfunc = (TF1*)getObject(&specFile, "bkgfunc", currentDirectory);
		if(bkgfunc) NbkgPar = bkgfunc->GetNpar();
		
		ampPar = NbkgPar + 3*peak;
		sigPar = NbkgPar + 3*peak + 2;

		TF1 *func = (TF1*)getObject(&specFile, "func", currentDirectory);
		dNdet = Ndet*std::sqrt(std::pow(func->GetParError(ampPar)/func->GetParameter(ampPar),2)+std::pow(func->GetParError(sigPar)/func->GetParameter(sigPar),2));
		
		/*fitresult = (TFitResult*)getObject(&specFile, "fitresult", currentDirectory);
		if(fitresult)
			dNdet = Ndet*std::sqrt(std::pow(fitresult->ParError(ampPar)/fitresult->Value(ampPar),2)+std::pow(fitresult->ParError(sigPar)/fitresult->Value(sigPar),2));*/

		// Get the effective intrinsic efficiency and uncertainty at the mean energy.
		effPtr->getEfficiency(En, intrinsic, dintrinsic);

		// Write output.
		outFile << peakCounts << "\t" << Ndet << "\t" << dNdet << "\t" << intrinsic << "\t" << dintrinsic << std::endl;

		delete peakIntrinsic;
		
	}

	specFile.Close();
	outFile.close();
	
	return true;
}

bool readEfficiency(const char *fname, const char *effname, const double &Ethresh=0.5){
	std::ifstream file(fname);
	if(!file.good()){
		std::cout << " Error! Failed to load input file \"" << fname << "\".\n";
		return false;
	}

	efficiencyFile eff(effname);
	if(eff.empty()){
		std::cout << " Error! Failed to open efficiency file \"" << effname << "\".\n";
		file.close();
		return false;
	}

	std::ofstream ofile("efficiency.out");
	ofile << "(dEff/Eff)\n";
	std::cout << "energy\teff\t(deff/eff)\n";

	double energy;
	double efficiency;
	double uncertainty;
	while(true){
		file >> energy >> efficiency;
		if(file.eof()) break;
		if(energy > Ethresh)
			eff.getUncertainty(Ethresh, efficiency, uncertainty);
		else
			eff.getUncertainty(0, efficiency, uncertainty);
		ofile << uncertainty/efficiency << std::endl;
		std::cout << energy << "\t" << efficiency << "\t" << uncertainty/efficiency << std::endl;
	}

	file.close();
	ofile.close();

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
	const std::string definedFunctions[52] = {"void", "binRatio", "TH1 *h1_, TH1 *h2_", "List the ratio of each bin in two 1-d histograms.",
	                                          "double", "calcEnergy", "const double &tof_", "Calculate neutron energy (MeV) given the time-of-flight (in ns).",
	                                          "double", "calcTOF", "const double &E_", "Calculate neutron time-of-flight (ns) given the energy (in MeV).",
	                                          "double", "TOFfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "TOFfitFunctions::landau", "double *x, double *p", "Standard landau (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "ENfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "ENfitFunctions::landau", "double *x, double *p", "Standard landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                          "double", "interpolate", "const double *py, const double &E, double &eff", "Use linear interpolation to calculate a value from a distribution.",
	                                          "void", "processMCarlo", "const char *fname, const int &comBins=18, const double &comLow=0, const double &comHigh=90", "Process a VANDMC monte carlo detector test output file.",
	                                          "void", "processMCarlo", "const char *fname, const char *binfname", "Process a VANDMC monte carlo output file and load CM bins from a file.",
	                                          "bool", "processSpecOutput", "const char *fname, const char *ofname, const char *effname, bool energy_=false", "Process an output file from specFitter (simpleScan tool).",

	                                          "bool", "readEfficiency", "const char *fname, const char *effname, const double &Ethresh=0.5", "Read efficiency values from an input file and report back the uncertainty.",
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
		for(int i = 0; i < 13; i++)
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
	
		for(int i = 0; i < 13; i++){
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
