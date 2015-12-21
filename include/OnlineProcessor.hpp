#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

#include <vector>
#include <string>

class Processor;
class Plotter;

class TCanvas;
class TPad;
class TApplication;

class OnlineProcessor{
  private:
	unsigned int canvas_cols;
	unsigned int canvas_rows;
  	unsigned int num_hists; /// The number of histograms which may be plotted at a given time.
  	
	int *which_hists; /// Array for storing which histogram to plot for each of the canvas pads.

	std::vector<Plotter*> plottable_hists; /// Vector of plottable histograms which is filled by the processors.

	TCanvas *can; /// Root canvas for plotting online data.
	TPad *pad; /// Pointer to the current TPad for drawing.
	
	TApplication *rootapp; /// Root application for handling graphics.
	
	TPad *cd(const unsigned int &index_);

  public:
  	/// Default constructor.
	OnlineProcessor(const unsigned int &cols_=2, const unsigned int &rows_=2);
	
	/// Destructor.
	~OnlineProcessor();
	
	/// Element access operator.
	Plotter* operator [] (const unsigned int &index_){ return GetPlot(index_); }
	
	Plotter* GetPlot(const unsigned int &index_);
	
	/// Change the histogram id of one of the canvas pads.
	bool ChangeHist(const unsigned int &index_, const unsigned int &hist_id_);

	/// Change the histogram id of one of the canvas pads.
	bool ChangeHist(const unsigned int &index_, const std::string &hist_name_);
	
	bool SetXrange(const unsigned int &index_, const double &xmin_, const double &xmax_);
	
	bool SetYrange(const unsigned int &index_, const double &ymin_, const double &ymax_);
	
	bool SetRange(const unsigned int &index_, const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_);
	
	bool ResetXrange(const unsigned int &index_);
	
	bool ResetYrange(const unsigned int &index_);
	
	bool ResetRange(const unsigned int &index_);
	
	bool ToggleLogX(const unsigned int &index_);
	
	bool ToggleLogY(const unsigned int &index_);
	
	bool ToggleLogZ(const unsigned int &index_);
	
	/// Refresh online plots.
	void Refresh();
	
	/// Add a processor's histograms to the list of plottable items.
	void AddHists(Processor *proc_);
	
	/// Add a single histogram to the list of plottable items.
	void AddHist(Plotter *hist_);
	
	/// Display a list of available plots.
	void PrintHists();
};

#endif
