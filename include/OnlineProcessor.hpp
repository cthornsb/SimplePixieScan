#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

#include <vector>
#include <deque>
#include <string>
#include <map>

class MapFile;
class Processor;
class Plotter;

class TCanvas;
class TPad;
class TH1;
class TFile;

class HistoDefFile{
  public:
	HistoDefFile(){ }

	HistoDefFile(const char *fname);

	bool ReadMap(const char *fname);

	bool GetNext(const std::string &type, std::string &line);
  
  private:
	std::map<std::string, std::deque<std::string> > histMap;
};

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
	
	TPad *cd(const unsigned int &index_);

	MapFile *mapfile;

	HistoDefFile histMap;

	std::string type;
	std::string name;
	int minloc;
	int maxloc;

	unsigned short histID;

	bool hadHistError;

  public:
  	/// Default constructor.
	OnlineProcessor();
	
	/// Destructor.
	~OnlineProcessor();

	/// Set pointer to the master detector map.
	MapFile *SetMapFile(MapFile *ptr_){ return (mapfile = ptr_); }
	
	/// Element access operator.
	Plotter* operator [] (const unsigned int &index_){ return GetPlot(index_); }
	
	Plotter* GetPlot(const unsigned int &index_);
	
	bool ReadHistMap(const char *fname);

	/// Return true if histograms are to be displayed using root TCanvas and false otherwise.
	bool DisplayMode(){ return display_mode; }

	/// Return true if at least one of the current processor's histograms failed to load.
	bool HadHistError(){ return hadHistError; }
	
	/// Activate display of histograms to TCanvas.
	bool SetDisplayMode(const unsigned int &cols_=2, const unsigned int &rows_=2);
	
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

	/** Zero a diagnostic histogram.
	  *  param[in]  hist_id_ Histogram ID index.
	  *  return True if the histogram exists and false otherwise.
	  */
	bool Zero(const unsigned int &hist_id_);
	
	/// Add a single histogram to the list of plottable items.
	void AddHist(Plotter *hist_);

	/// Prepare to add a processor's histograms to the list of plottable items.
	void StartAddHists(Processor *proc_);

	/// Generate a new histogram and add it to the list of plottable items.
	bool GenerateHist(Plotter* &hist_);

	/// Generate and add a location histogram to the list of plottable items.
	void GenerateLocationHist(Plotter* &hist_);
	
	/// Write all histograms to a root TTree.
	int WriteHists(TFile *file_, const std::string &dirname_="hists");
	
	/// Display a list of available plots.
	void PrintHists();
};

#endif
