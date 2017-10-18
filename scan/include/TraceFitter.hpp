#ifndef TRACE_FITTER_HPP
#define TRACE_FITTER_HPP

#include "XiaData.hpp"

/**The Paulauskas function is described in NIM A 737 (22), with a slight 
 * adaptation. We use a step function such that f(x < phase) = baseline.
 * In addition, we also we formulate gamma such that the gamma in the paper is
 * gamma_prime = 1 / pow(gamma, 0.25).
 *
 * The parameters are:
 * p[0] = baseline
 * p[1] = amplitude
 * p[2] = phase
 * p[3] = beta
 * p[4] = gamma
 *
 * \param[in] x X value.
 * \param[in] p Paramater values.
 *
 * \return the value of the function for the specified x value and parameters.
 */
double paulauskas(double *x, double *p);

class TF1;
class TH1D;
class TGraph;

class TraceFitter{
  protected:
	TF1 *func;
	
	size_t fittingLow;
	size_t fittingHigh;
	
	double beta;
	double gamma;

	double xAxisMult;

	bool floatingMode;

  public:
	TraceFitter();

	TraceFitter(const char* func_);
		
	TraceFitter(double (*func_)(double *, double *), int npar_);

	~TraceFitter();

	/// Return a pointer to the TF1 parameter array.
	double *GetParameters();

	/// Return a pointer to the root TF1 function.
	TF1 *GetFunction(){ return func; }
	
	/// Set the range of the fit as [maxIndex-low_, maxIndex+high_].
	bool SetFitRange(const size_t &low_, const size_t &high_);
	
	/// Set the fixed beta and gamma parameters.
	bool SetBetaGamma(const double &beta_, const double &gamma_);
	
	/// Set all fit parameters to floating.
	bool SetFloatingMode(const bool &mode_=true){ return (floatingMode = mode_); }

	/// Set the x-axis multiplier constant for fitting graphs and histograms.
	double SetAxisMultiplier(const double &mult_){ return (xAxisMult = mult_); }

	/// Set the initial conditions for the fit.
	bool SetInitialConditions(ChannelEvent *event_);

	/// Fit a single trace.
	bool FitPulse(ChannelEvent *event_, const char *fitOpt="QR");

	/// Fit a root TGraph.
	bool FitPulse(TGraph *graph_, ChannelEvent *event_, const char *fitOpt="QR");

	/// Fit a root TH1D..
	bool FitPulse(TH1D *hist_, ChannelEvent *event_, const char *fitOpt="QR");
};

#endif
