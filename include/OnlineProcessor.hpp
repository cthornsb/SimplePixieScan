#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

#include <vector>
#include <deque>
#include <string>
#include <utility>
#include <map>

#include "TApplication.h"

class MapFile;
class Processor;
class Plotter;

class TCanvas;
class TPad;
class TH1;
class TFile;

/** @class HistoDefFile
  * @author Cory R. Thornsberry
  * @date September 16, 2019
  * @brief Reads histogram definitions from an input file
  */
class HistoDefFile{
  public:
	/** Default constructor
	  */
	HistoDefFile(){ }

	/** File reading constructor
	  */
	HistoDefFile(const char *fname);

	/** Read a histogram definition map
	  * @param fname Path to the input ascii file containing the histogram definitions
	  * @return True if the map is read correctly and return false otherwise
	  */
	bool ReadMap(const char *fname);

	/** Get the next histogram definition in the list for a specified detector type
	  * @param type The name of the detector type for which the next histogram definition will be retrieved
	  * @param line The next histogram definition line for the specified detector type
	  * @return True if another definition exists for the specified detector type and return false otherwise
	  */
	bool GetNext(const std::string &type, std::string &line);
  
  private:
	std::map<std::string, std::deque<std::string> > histMap; ///< The map of deques of histogram definitions for named detector types
};

/** @class OnlineProcessor
  * @author Cory R. Thornsberry
  * @date September 16, 2019
  * @brief Handles online histograms, plotting, and ROOT graphics
  */
class OnlineProcessor : public TApplication {
  private:
	unsigned int canvas_cols;
	unsigned int canvas_rows;
  	unsigned int num_pads; ///< The number of canvas pads which are available on the canvas
  	
  	bool display_mode; ///< True if histograms are to be displayed in a root canvas
	bool hadHistError;

	TCanvas *can; ///< Root canvas for plotting online data
	TPad *pad; ///< Pointer to the current TPad for drawing
	Plotter *plot; ///< Pointer to the current plotter object
	
	MapFile *mapfile;

	HistoDefFile histMap;

	std::string type; ///< The type 
	std::string name;
	int minloc;
	int maxloc;

	unsigned short histID; ///< The ID number of the next histogram which will be generated (starting from 1)

	std::vector<std::pair<int, int> > which_hists; ///< Vector for storing which histogram (and sub-histogram) to plot for each of the canvas pads

	std::vector<Plotter*> plottable_hists; ///< Vector of plottable histograms which is filled by the processors

	std::vector<int> locations; ///< Vector of detector IDs from the input map file

	/** Select the current TPad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return A pointer to the selected TPad and return NULL in the event that it does not exist
	  */
	TPad *cd(const unsigned int &index_);

  public:
  	/** Default constructor
	  */
	OnlineProcessor();
	
	/** Destructor
	  */
	~OnlineProcessor();

	/** Set pointer to the master detector map
	  */
	MapFile *SetMapFile(MapFile *ptr_){ return (mapfile = ptr_); }
	
	/** Element access operator
	  */
	Plotter* operator [] (const unsigned int &index_){ return GetPlot(index_); }
	
	/** Get a pointer to the histogram at a specified index in the list of all histograms
	  */
	Plotter* GetPlot(const unsigned int &index_);

	/** Return the number of histograms which are currently defined
	  */
	unsigned int GetNumHistograms() const { return plottable_hists.size(); }

	/** Return the number of canvas pads which are available on the canvas
	  */ 
	unsigned int GetNumCanvasPads() const { return num_pads; }

	/** Read the detector map
	  * @param fname The path to the detector map
	  * @return True if the map is read successfully and return false otherwise
	  */	
	bool ReadHistMap(const char *fname);

	/** Return true if the TCanvas has been initialized and return false otherwise
	  */
	bool DisplayMode(){ return display_mode; }

	/** Return true if one or more of the current processor's histograms failed to load
	  */
	bool HadHistError(){ return hadHistError; }
	
	/** Initialize the TCanvas
	  * @param cols_ The number of columns to divide the canvas into
	  * @param rows_ The number of rows to divide the canvas into
	  * @return True if the display mode is set correctly and return false if it has already been initialized
	  */
	bool SetDisplayMode(const unsigned int &cols_=2, const unsigned int &rows_=2);
	
	/** Change the histogram id of one of the canvas pads
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @param hist_id_ Index of the requested histogram in the list of histograms
	  * @parma det_id_ The ID number of the detector whose sub-histogram is requested (if negative, the histogram for all detectors is used)
	  * @return True if the requested pad location and histogram exists and return false otherwise
	  */
	bool ChangeHist(const unsigned int &index_, const unsigned int &hist_id_, const int &det_id_=-1);

	/** Change the histogram id of one of the canvas pads by name
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @param hist_name_ The name of the  histogram to select from the list
	  * @return True if the requested pad location and histogram exists and return false otherwise
	  */
	bool ChangeHist(const unsigned int &index_, const std::string &hist_name_);
	
	/** Set the X-axis range for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @param xmin_ The new minimum
	  * @param xmax_ The new maximum
	  * @return True if the pad index exists and the range is valid and return false otherwise
	  */
	bool SetXrange(const unsigned int &index_, const double &xmin_, const double &xmax_);

	/** Set the Y-axis range for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @param ymin_ The new minimum
	  * @param ymax_ The new maximum
	  * @return True if the pad index exists and the range is valid and return false otherwise
	  */	
	bool SetYrange(const unsigned int &index_, const double &ymin_, const double &ymax_);

	/** Set the X-axis and Y-axis ranges for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @param xmin_ The new minimum of the X-axis
	  * @param xmax_ The new maximum of the X-axis
	  * @param ymin_ The new minimum of the Y-axis
	  * @param ymax_ The new maximum of the Y-axis
	  * @return True if the pad index exists and both ranges are valid and return false otherwise
	  */	
	bool SetRange(const unsigned int &index_, const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_);
	
	/** Reset the X-axis range for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */
	bool ResetXrange(const unsigned int &index_);

	/** Reset the Y-axis range for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */	
	bool ResetYrange(const unsigned int &index_);

	/** Reset the X-axis and Y-axis ranges for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */
	bool ResetRange(const unsigned int &index_);
	
	/** Toggle logarithmic X-axis scale for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */
	bool ToggleLogX(const unsigned int &index_);

	/** Toggle logarithmic Y-axis scale for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */	
	bool ToggleLogY(const unsigned int &index_);

	/** Toggle logarithmic Z-axis scale for the histogram currently displayed on a specified pad
	  * @param index_ The index of the TPad to select (zero-indexed)
	  * @return True if the pad index exists and return false otherwise
	  */	
	bool ToggleLogZ(const unsigned int &index_);

	/** Refresh a single histogram currently drawn on the canvas
	  * @param index_ The index of the TPad to select (zero-indexed)
	  */
	void Refresh(const unsigned int &index_);
	
	/** Refresh all histograms currently drawn on the canvas
	  */
	void Refresh();

	/** Zero one of the histograms in the list
	  * @param hist_id_ Index of the requested histogram in the list of histograms
	  * @return True if the histogram exists and return false otherwise
	  */
	bool Zero(const unsigned int &hist_id_);
	
	/** Add a single histogram to the list of plottable items
	  */
	void AddHist(Plotter *hist_);

	/** Prepare to add a processor's histograms to the list of plottable items
	  */
	void StartAddHists(Processor *proc_);

	/** Generate a new histogram and add it to the list of plottable items
	  */
	bool GenerateHist(Plotter* &hist_);

	/** Generate and add a location histogram to the list of plottable items
	  */
	void GenerateLocationHist(Plotter* &hist_);
	
	/** Write all histograms to a root TTree
	  * @param file_ Pointer to the file to write to (we assume it is writable)
	  * @param dirname_ The name of the TDirectory where the histograms will be written
	  */
	int WriteHists(TFile *file_, const std::string &dirname_="hists");
	
	/** Display a list of available plots
	  */
	void PrintHists();
};

#endif
