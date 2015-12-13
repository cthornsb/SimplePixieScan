#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

#include <vector>
#include <string>

class Processor;

class TH1;
class TCanvas;
class TApplication;

class PlotObject{
  public:
	std::string opt;
	std::string name;
	std::string title;
	
	TH1 *hist;
	
	PlotObject(TH1 *hist_, const std::string &draw_opt_="");

	~PlotObject(){ }
	
	void Draw();
};

class OnlineProcessor{
  private:
	unsigned int canvas_cols;
	unsigned int canvas_rows;
  	unsigned int num_hists; /// The number of histograms which may be plotted at a given time.
  	
	int *which_hists; /// Array for storing which histogram to plot for each of the canvas pads.

	std::vector<PlotObject*> plottable_hists; /// Vector of plottable histograms which is filled by the processors.

	TCanvas *can; /// Root canvas for plotting online data.
	
	TApplication *rootapp; /// Root application for handling graphics.
	
  public:
  	/// Default constructor.
	OnlineProcessor(const unsigned int &cols_=2, const unsigned int &rows_=2);
	
	/// Destructor.
	~OnlineProcessor();
	
	/// Change the histogram id of one of the canvas pads.
	bool ChangeHist(const unsigned int &index_, const unsigned int &hist_id_);

	/// Change the histogram id of one of the canvas pads.
	bool ChangeHist(const unsigned int &index_, const std::string &hist_name_);
	
	/// Refresh online plots.
	void Refresh();
	
	/// Add a processor's histograms to the list of plottable items.
	void AddHists(Processor *proc_);
	
	/// Add a single histogram to the list of plottable items.
	void AddHist(PlotObject *hist_);
	
	/// Display a list of available plots.
	void PrintHists();
};

#endif
