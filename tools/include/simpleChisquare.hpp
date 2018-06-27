#ifndef SIMPLE_CHISQUARE_HPP
#define SIMPLE_CHISQUARE_HPP

#include "simpleTool.hpp"

// These need to be global.
extern size_t Nelements;
extern double *xval, *yval;

double comp(double *x, double *p);

double comp2(double *x, double *p);

class TGraphErrors;

class chisquare : public simpleTool {
  public:
	chisquare();

	~chisquare();

	double setInitialParameter(const double &initialParameter_){ return (initialParameter=initialParameter_); }

	void setFitRange(const double &low_, const double &high_);

	void setParLimits(const double &low_, const double &high_);

	bool setAddConstTerm(const bool &state_=true){ return (addConstTerm=state_); }

	void setTheoreticalFile(const std::string &fname_, const std::string &gname_);

	void setExperimentalFile(const std::string &fname_, const std::string &gname_);

	TGraphErrors *setTheoreticalGraph(TGraphErrors *g_){ return (gT=g_); }

	TGraphErrors *setExperimentalGraph(TGraphErrors *g_){ return (gE=g_); }

	double getInitialParameter(){ return initialParameter; }

	void getFitRange(double &low_, double &high_);

	void getParLimits(double &low_, double &high_);

	bool getAddConstTerm(){ return addConstTerm; }

	double getA(){ return A; }

	double getB(){ return B; }

	double getChi2(){ return chi2; }

	void addOptions();
	
	bool processArgs();
	
	int execute(int argc, char *argv[]);

	bool compute();

  protected:
	double initialParameter;
	double fitRangeLow;
	double fitRangeHigh;
	double parLimitLow;
	double parLimitHigh;
	
	bool userFitRange;
	bool userParLimits;
	bool addConstTerm;
	bool loadedGraphs;
	
	TGraphErrors *gT;
	TGraphErrors *gE;

	double A, B, chi2;

	std::string fnames[2];
	std::string gnames[2];

	bool load();
};

#endif
