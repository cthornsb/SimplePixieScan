// helper.cpp
// Author: Cory R. Thornsberry
// Updated: May 18th, 2017

#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////

const double Mn = 10454.0750977429; // Mass of neutron, in MeV.
const double pi = 3.1415926536; // pi constant.
const double twopi = 6.2831853072; // 2*pi constant.

// Array of root colors.
const short colors[6] = {kBlue+2, kRed, kGreen+2, kOrange+7, kMagenta+2, kCyan+2};

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
const double effBa157[44] = {0.15, -7.751, -1.251, -0.540, -0.243, -0.155, -0.015, 0.001, 0.033, 0.077, 
                             0.143, 0.223, 0.312, 0.358, 0.371, 0.379, 0.380, 0.380, 0.371, 0.370, 
                             0.362, 0.353, 0.343, 0.328, 0.344, 0.326, 0.311, 0.311, 0.303, 0.300, 
                             0.291, 0.289, 0.284, 0.275, 0.282, 0.261, 0.265, 0.263, 0.250, 0.242, 
                             0.230, 0.221, 0.218, 0.174};

// Array of Americium threshold efficiencies for use in efficiency interpolation.
const double effAm327[44] = {0.24, -7.590, -1.508, -0.653, -0.358, -0.223, -0.085, -0.060, -0.025, -0.004, 
                             0.006, 0.007, 0.007, 0.024, 0.064, 0.119, 0.170, 0.214, 0.235, 0.262, 
                             0.266, 0.267, 0.267, 0.263, 0.281, 0.267, 0.263, 0.266, 0.262, 0.263, 
                             0.259, 0.261, 0.256, 0.248, 0.256, 0.253, 0.242, 0.243, 0.232, 0.224, 
                             0.213, 0.204, 0.198, 0.158};

const double olaCorrEff[49] = {0.1274000396, 0.012436848, 0.0315360447, 0.0570962032, 0.0855405784, 0.1136005537, 0.1412761292, 0.1689517046, 0.19662728, 0.2246872553, 0.2531316305, 
                               0.2801659307, 0.3019473743, 0.3188597525, 0.3373097303, 0.3542221085, 0.3726720863, 0.3892000646, 0.4050873764, 0.4241743684, 0.4443846726, 
                               0.4493693319, 0.44191695, 0.4225882243, 0.4085663866, 0.3989486739, 0.3897096847, 0.3810205339, 0.372331383, 0.3634083683, 0.3539771791, 
                               0.3451647755, 0.3360836238, 0.3278917806, 0.3177546408, 0.3110764915, 0.3063215591, 0.3014384934, 0.2964266856, 0.2889258657, 0.2814070889, 
                               0.2742910167, 0.2684525383, 0.2654708195, 0.2623778807, 0.2577508207, 0.2531256449, 0.2468635229, 0.2431058846};

const double olaCorrEnergy[49] = {-9999, 0.1274000396, 0.1824030087, 0.2068487728, 0.2190716548, 0.2312945368, 0.2435174188, 0.2557403009, 0.2679631829, 0.2801860649, 0.292408947, 
                                  0.31074327, 0.322966152, 0.335189034, 0.3474119161, 0.3596347981, 0.3718576801, 0.3840805622, 0.3963034442, 0.4451949723, 0.5368665875,
                                  0.6627622724, 0.8057699921, 0.9325823931, 1.0590552696, 1.1977752799, 1.3322269822, 1.4596941805, 1.5871613788, 1.7194304236, 1.8538821259, 
                                  1.9782936037, 2.131952692, 2.2786272763, 2.4253018606, 2.578087886, 2.7186510293, 2.8592141726, 3.0058887569, 3.1481980262, 3.2870150435, 
                                  3.4258320609, 3.6021907872, 3.7663266316, 3.944431484, 4.0876138163, 4.2118797836, 4.3687401029, 4.4665231591};

const double *effPtr = effZero;
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

// Convert logic signal period to beam current (enA).
void conv(double &x, double &y){
	x *= 8E-9;
	y = 1E8/(8*y);
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
TGraph *convert(TGraph *g, const char *name){
	TGraph *output = (TGraph*)(g->Clone(name));

	double x, y;
	for(int i = 0; i < g->GetN(); i++){
		g->GetPoint(i, x, y);
		conv(x, y);
		output->SetPoint(i, x, y);
	}

	return output;
}

// Return the range of bins containing an upper and lower point, rounded to the nearest bins.
void findBins(TH1 *h, const double &xstart_, const double &xstop_, int &lowBin, int &highBin){
	if(xstart_ <= h->GetXaxis()->GetXmin()) lowBin = 1;
	else                                    lowBin = h->FindBin(xstart_);
	if(xstop_ >= h->GetXaxis()->GetXmax())  highBin = h->GetNbinsX();
	else                                    highBin = h->FindBin(xstop_);
}

// Return the total integral of a logic signal TGraph.
double integrate(TGraph *g){
	double sum = 0.0;
	double integral = 0.0;
	double x1, y1;
	double x2, y2;
	g->GetPoint(0, x1, y1);
	conv(x1, y1);
	sum += y1;
	for(int i = 1; i < g->GetN(); i++){
		g->GetPoint(i, x2, y2);
		conv(x2, y2);
		sum += y2;
		integral += 0.5 * (y1 + y2) * (x2 - x1);
		x1 = x2;
		y1 = y2;
	}
	std::cout << "mean = " << sum/g->GetN() << " enA, integral = " << integral << " nC\n";
	return integral;
}

// Return the total integral of a 1-d histogram.
//  NOTE: This function returns the integral of the histogram, not the number of counts.
double integrate(TH1 *h){
	double sum = 0;
	for(int i = 1; i < h->GetNbinsX(); i++){
		sum += 0.5 * (h->GetBinContent(i) + h->GetBinContent(i+1)) * h->GetBinWidth(i);
	}
	return sum;
}

// Return the integral of a 1-d histogram in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the integral of the histogram, not the number of counts.
double integrate(TH1 *h, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	for(int i = lowBin; i < highBin; i++)
		retval += 0.5 * (h->GetBinContent(i) + h->GetBinContent(i+1)) * h->GetBinWidth(i);
	return retval;
}

// Integrate a root function in the range [low, high] given parameter array p.
double integrate(const char *funcstr, double *p, double low, double high){
	TF1 *func = new TF1("func", funcstr, low, high);
	func->SetParameters(p);
	double retval = func->Integral(low, high);
	delete func;
	return retval;
}

// Calculate a 2nd order polynomial that passes through three (x,y) pairs.
void calculateP2(double *x, double *y, double *p){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		x1[i] = x[i];
		x2[i] = std::pow(x[i], 2);
	}

	double denom = (x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(x2[2]-x2[1]*1) + x2[0]*(x1[2]-x1[1]*1);

	p[0] = (float)((y[0]*(x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(y[1]*x2[2]-x2[1]*y[2]) + x2[0]*(y[1]*x1[2]-x1[1]*y[2]))/denom);
	p[1] = (float)(((y[1]*x2[2]-x2[1]*y[2]) - y[0]*(x2[2]-x2[1]*1) + x2[0]*(y[2]-y[1]*1))/denom);
	p[2] = (float)(((x1[1]*y[2]-y[1]*x1[2]) - x1[0]*(y[2]-y[1]*1) + y[0]*(x1[2]-x1[1]*1))/denom);
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
	
	return sum*8*1E-9;
}

// Return the total number of counts in a 1-d histogram.
//  NOTE: This function returns the number of counts in a histogram, not the integral.
double summation(TH1 *h){
	return h->Integral();
}

// Return the number of counts in a 1-d histogram in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts in a histogram, not the integral.
double summation(TH1 *h, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	for(int i = lowBin; i <= highBin; i++)
		retval += h->GetBinContent(i);
	return retval;
}

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

// Return the number of counts under a TF1 in the range [low, high], rounded to the nearest bins.
//  NOTE: This function returns the number of counts under a curve, not the function integral.
double summation(TH1 *h, TF1 *f, const double &xstart_, const double &xstop_){
	int lowBin, highBin;
	findBins(h, xstart_, xstop_, lowBin, highBin);
	double retval = 0;
	double xlo, xhi;
	for(int i = lowBin; i <= highBin; i++){
		xlo = h->GetBinLowEdge(i);
		xhi = xlo + h->GetBinWidth(i);
		retval += f->Integral(xlo, xhi)/(xhi-xlo);
	}
	return retval;
}

// Multiply a 1-d histogram by a constant.
void multiply(TH1 *h_, const double &c_){
	for(int i = 1; i <= h_->GetNbinsX(); i++){
		h_->SetBinContent(i, h_->GetBinContent(i)*c_);
	}
}

// Multiply a 2-d histogram by a constant.
void scale(TH2F *h, const double &scaling){
	int gbin;
	for(int i = 1; i <= h->GetXaxis()->GetNbins(); i++){
		for(int j = 1; j <= h->GetYaxis()->GetNbins(); j++){
			gbin = h->GetBin(i, j);
			h->SetBinContent(gbin, h->GetBinContent(gbin)*scaling);
		}
	}
}

// List all bins of a 1-d histogram.
void listBins(TH1 *h_, const double &c_=1){
	double sum = 0;
	for(int i = 1; i <= h_->GetNbinsX(); i++){
		sum += h_->GetBinContent(i)*c_;
		std::cout << i << "\t" << h_->GetBinContent(i)*c_ << std::endl;
	}
	std::cout << " total = " << sum << std::endl;
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

// Add a second x-axis to a TCanvas.
TGaxis *extraXaxis(){
	TGaxis *axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUxmax(),gPad->GetUymax(),0,100,510,"+L");
	axis->Draw();
	return axis;
}

// Add a second y-axis to a TCanvas.
TGaxis *extraYaxis(){
	TGaxis *axis = new TGaxis(gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymax(),0,100,510,"+L");
	axis->Draw();
	return axis;
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
		binID = strtol(currentDirectory.substr(currentDirectory.find("bin")+3).c_str(), NULL, 0);

		currentDirectory += "/";

		// Load the fit function from the file.
		func = (TF1*)getObject(&specFile, "func", currentDirectory);

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

// Set an object which inherits from TAttLine to have user specified style.
void setLine(TAttLine *ptr_, const short &color_=602, const short &style_=1, const short &width_=1){
	if(!ptr_) return;
	ptr_->SetLineColor(color_);
	ptr_->SetLineStyle(style_);
	ptr_->SetLineWidth(width_);
}

// Set an object which inherits from TAttMarker to have user specified style.
void setMarker(TAttMarker *ptr_, const short &color_=602, const short &style_=21, const float &size_=1.0){
	if(!ptr_) return;
	ptr_->SetMarkerColor(color_);
	ptr_->SetMarkerStyle(style_);
	ptr_->SetMarkerSize(size_);
}

void help(const std::string &search_=""){
	// Colored terminal character string.
	const std::string dkred("\033[1;31m");
	const std::string dkblue("\033[1;34m");
	const std::string reset("\033[0m");

	// Defined global constants.
	const std::string globalConstants[24] = {"const double", "Mn", "Mass of neutron, in MeV.",
	                                         "const double", "pi", "pi constant.",
	                                         "const double", "twopi", "2*pi constant.",
	                                         "const short", "colors[6]", "A`rray of root colors.",
	                                         "const double", "effEnergy[44]", "Array of energies for use in efficiency interpolation.",
	                                         "const double", "effZero[44]", "Array of zero-threshold efficiencies for use in efficiency interpolation.",
	                                         "const double", "effBa157[44]", "Array of Barium threshold efficiencies for use in efficiency interpolation.",
	                                         "const double", "effAm327[44]", "Array of Americium threshold efficiencies for use in efficiency interpolation."};

	// Defined global variables.
	const std::string globalVariables[9] = {"double", "d", "Distance from target to detector, in m.",
	                                        "double", "dd", "Thickness of detector, in m.",
	                                        "double", "dt", "Timing resolution of detector, in ns."};

	// Defined functions.
	const std::string definedFunctions[132] = {"void", "binRatio", "TH1 *h1_, TH1 *h2_", "List the ratio of each bin in two 1-d histograms.",
	                                           "double", "calcEnergy", "const double &tof_", "Calculate neutron energy (MeV) given the time-of-flight (in ns).",
	                                           "double", "calcTOF", "const double &E_", "Calculate neutron time-of-flight (ns) given the energy (in MeV).",
	                                           "void", "calculateP2", "double *x, double *y, double *p", "Calculate a 2nd order polynomial that passes through three (x,y) pairs.",
	                                           "void", "conv", "double &x, double &y", "Convert logic signal period to beam current (enA).",
	                                           "TGraph", "convert", "TGraph *g, const char *name", "Convert a logic signal TGraph to a beam current vs. time graph.",
	                                           "double", "efficiency", "double *x, double *p", "Piecewise defined VANDLE intrinsic efficiency as a function of neutron ToF.",
	                                           "double", "effVsEnergy", "double *x, double *p", "Piecewise defined VANDLE intrinsic efficiency as a function of neutron Energy.",
	                                           "TGaxis", "extraXaxis", "", "Add a second x-axis to a TCanvas.",
	                                           "TGaxis", "extraYaxis", "", "Add a second y-axis to a TCanvas.",
	                                           "void", "findBins", "TH1 *h, const double &xstart_, const double &xstop_, int &lowBin, int &highBin", "Return the range of bins containing an upper and lower point, rounded to the nearest bins.",
	                                           "double", "TOFfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                           "double", "TOFfitFunctions::landau", "double *x, double *p", "Standard landau (x in ns) scaled by linearly interpolated intrinsic efficiency.",
	                                           "double", "ENfitFunctions::gaussian", "double *x, double *p", "Standard gaussian (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                           "double", "ENfitFunctions::landau", "double *x, double *p", "Standard landau (x in MeV) scaled by linearly interpolated intrinsic efficiency.",
	                                           "double", "integrate", "TGraph *g", "Return the total integral of a logic signal TGraph.",
	                                           "double", "integrate", "TH1 *h", "Return the total integral of a 1-d histogram.",
	                                           "double", "integrate", "TH1 *h, const double &xstart_, const double &xstop_", "Return the integral of a 1-d histogram in the range [low, high], rounded to the nearest bins.",
	                                           "double", "integrate", "const char *funcstr, double *p, double low, double high", "Integrate a root function in the range [low, high] given parameter array p.",
	                                           "double", "interpolate", "const double *py, const double &E, double &eff", "Use linear interpolation to calculate a value from a distribution.",
	                                           "void", "listBins", "TH1 *h_, const double &c_=1", "List all bins of a 1-d histogram.",
	                                           "double", "mean", "TGraph *g", "Determine the mean beam current from a logic signal TGraph.",
	                                           "void", "multiply", "TH1 *h_, const double &c_", "Multiply a 1-d histogram by a constant.",
	                                           "void", "processMCarlo", "const char *fname, const char *tname=\"data\"", "Process a VANDMC monte carlo detector test output file.",
	                                           "bool", "processSpecOutput", "const char *fname, const char *ofname, const double *ptr_=effZero, bool energy_=false", "Process an output file from specFitter (simpleScan tool).",
	                                           "void", "scale", "TH2F *h, const double &scaling", "Multiply a 2-d histogram by a constant.",
	                                           "void", "setLine", "TAttLine *ptr_, const short &color_=602, const short &style_=1, const short &width_=1", "Set an object which inherits from TAttLine to have user specified style.",
	                                           "void", "setMarker", "TAttMarker *ptr_, const short &color_=602, const short &style_=21, const float &size_=1.0", "Set an object which inherits from TAttMarker to have user specified style.",
	                                           "double", "summation", "TTree *t", "Return the total run time in seconds.",
	                                           "double", "summation", "TH1 *h", "Return the total number of counts in a 1-d histogram.",
	                                           "double", "summation", "TH1 *h, const double &xstart_, const double &xstop_", "Return the number of counts in a 1-d histogram in the range [low, high], rounded to the nearest bins.",
	                                           "double", "summation", "TH1 *h, TF1 *f", "Return the total number of counts under a TF1.",
	                                           "double", "summation", "TH1 *h, TF1 *f, const double &xstart_, const double &xstop_", "Return the number of counts under a TF1 in the range [low, high], rounded to the nearest bins."};

	if(search_.empty()){
		std::cout << "*****************************\n";
		std::cout << "**          HELP           **\n";
		std::cout << "*****************************\n";
	
		std::cout << " Defined global constants:\n";
		for(int i = 0; i < 8; i++)
			std::cout << "  " << globalConstants[3*i+1] << std::endl;
	
		std::cout << "\n Defined global variables:\n";
		for(int i = 0; i < 3; i++)
			std::cout << "  " << globalVariables[3*i+1] << std::endl;
	
		std::cout << "\n Defined helper functions:\n";
		for(int i = 0; i < 33; i++)
			std::cout << "  " << definedFunctions[4*i+1] << std::endl;
			
		std::cout << std::endl;
	}
	else{
		size_t fIndex;
		std::string strings[3];
		
		for(int i = 0; i < 8; i++){
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
	
		for(int i = 0; i < 33; i++){
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

int helper(){
	std::cout << " helper.cpp\n";
	std::cout << "  NOTE - type 'help()' to display a list of commands.\n";
	std::cout << "  NOTE - or 'help(string)' to search for a command or variable.\n";
	return 0;
}
