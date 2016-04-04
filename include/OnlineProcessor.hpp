#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

#include <vector>
#include <string>

class Processor;
class Plotter;

class TCanvas;
class TPad;
class TH1;
class TFile;
class TApplication;

class OnlineProcessor{
  private:
	unsigned int canvas_cols;
	unsigned int canvas_rows;
  	unsigned int num_hists; /// The number of histograms which may be plotted at a given time.
  	
  	bool display_mode; /// True if histograms are to be displayed in a root canvas.
  	
	int *which_hists; /// Array for storing which histogram to plot for each of the canvas pads.

	std::vector<Plotter*> plottable_hists; /// Vector of plottable histograms which is filled by the processors.

	TCanvas *can; /// Root canvas for plotting online data.
	TPad *pad; /// Pointer to the current TPad for drawing.
	Plotter *plot; /// Pointer to the current plotter object.
	
	TApplication *rootapp; /// Root application for handling graphics.
	
	TPad *cd(const unsigned int &index_);

  public:
  	/// Default constructor.
	OnlineProcessor();
	
	/// Destructor.
	~OnlineProcessor();
	
	/// Element access operator.
	Plotter* operator [] (const unsigned int &index_){ return GetPlot(index_); }
	
	Plotter* GetPlot(const unsigned int &index_);
	
	/// Return true if histograms are to be displayed using root TCanvas and false otherwise.
	bool DisplayMode(){ return display_mode; }
	
	/// Activate display of histograms to TCanvas.
	void SetDisplayMode(const unsigned int &cols_=2, const unsigned int &rows_=2);
	
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

	/// Refresh a single online plot.
	void Refresh(const unsigned int &index_);
	
	/// Refresh all online plots.
	void Refresh();
	
	/// Add a processor's histograms to the list of plottable items.
	void AddHists(Processor *proc_);
	
	/// Add a single histogram to the list of plottable items.
	void AddHist(Plotter *hist_);
	
	/// Write all histograms to a root TTree.
	int WriteHists(TFile *file_, const std::string &dirname_="hists");
	
	/// Display a list of available plots.
	void PrintHists();
};

#endif
