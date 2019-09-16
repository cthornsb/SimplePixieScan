#ifndef PLOTTER_HPP
#define PLOTTER_HPP

#include <string>
#include <vector>
#include <utility>

class TH1;
class TPad;
class TFile;

class Plotter{
  private:
	int dim; ///< Dimension of the main ROOT histogram
	int xbins; ///< Number of bins along the x-axis
	int ybins; ///< Number of bins along the y-axis

	double xmin; ///< Minimum value along the x-axis
	double xmax; ///< Maximum value along the x-axis
	double ymin; ///< Minimum value along the y-axis
	double ymax; ///< Maximum value along the y-axis

	bool logx; ///< Flag indicating that the TCanvas x-axis is in logarithmic scale
	bool logy; ///< Flag indicating that the TCanvas y-axis is in logarithmic scale
	bool logz; ///< Flag indicating that the TCanvas z-axis is in logarithmic scale
	
	std::string name; ///< Name of the main ROOT histogram (and prefix of the secondary histograms)
	std::string title; ///< Title of all ROOT histograms
	std::string opt; ///< ROOT draw option
	
	std::string xtitle; ///< Title of the x-axis
	std::string ytitle; ///< Title of the y-axis
	std::string xunits; ///< Unit name used for the x-axis
	std::string yunits; ///< Unit name used for the y-axis
	
	size_t numHists; ///< Number of secondary histograms
	
  	TH1* hist; ///< Pointer to the main ROOT histogram
  	
	std::vector<std::pair<TH1*, int> > hists1d; ///< Vector of pairs of secondary histograms and their corresponding detector ID
	
  public:
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_, 
	        const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_);
	
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_,
	        const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_, const std::string &ytitle_,
	        const std::string &yunits_, const int &ybins_, const double &ymin_, const double &ymax_);
	
	~Plotter();
	
	TH1 *AddNew1dHistogram(const int &location, const std::string &newTitle="");
	
	TH1 *AddNew2dHistogram(const int &location, const std::string &newTitle="");
	
	TH1 *GetHist(){ return hist; }
	
	TH1 *GetHist(const int &location);
	
	int GetNdim(){ return dim; }
	
	double GetXmin(){ return xmin; }
	
	double GetXmax(){ return xmax; }
	
	double GetYmin(){ return ymin; }
	
	double GetYmax(){ return ymax; }

	bool GetLogX(){ return logx; }
	
	bool GetLogY(){ return logy; }
	
	bool GetLogZ(){ return logz; }
	
	std::string GetName(){ return name; }
	
	std::string GetDrawOption(){ return opt; }

	void GetXrange(double &xmin_, double &xmax_);
	
	void GetYrange(double &ymin_, double &ymax_);
	
	void GetRange(double &xmin_, double &xmax_, double &ymin_, double &ymax_);
	
	size_t GetNumHists() const { return numHists; }
	
	bool DetectorIsDefined(const int &id) const ;
	
	void SetNdim(const int &N){ dim = N; }
	
	void SetXaxisTitle(const std::string &title_);
	
	void SetYaxisTitle(const std::string &title_);
	
	void SetStats(const bool &state_=true);
	
	void SetXrange(const double &xmin_, const double &xmax_);
	
	void SetYrange(const double &ymin_, const double &ymax_);
	
	void SetRange(const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_);

	void SetupHist1d(TH1 *h);
	
	void SetupHist2d(TH1 *h);
	
	void ResetXrange();
	
	void ResetYrange();
	
	void ResetRange();
	
	void ToggleLogX(){ logx = !logx; }
	
	void ToggleLogY(){ logy = !logy; }
	
	void ToggleLogZ(){ logz = !logz; }
	
	void Zero();

	void Fill(const double &x_);
	
	void Fill(const int &detID, const double &x_);

	void Fill2d(const double &x_, const double &y_);
	
	void Fill2d(const int &detID, const double &x_, const double &y_);
	
	void Draw(TPad *pad_, const int &detID=-1);
	
	void Write(TFile *file_, const std::string &dirname_="hists");
	
	void Print();
};

#endif
