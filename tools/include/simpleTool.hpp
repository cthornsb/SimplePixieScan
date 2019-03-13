#ifndef SIMPLETOOL_HPP
#define SIMPLETOOL_HPP

#include <vector>
#include <deque>
#include <string>
#include <getopt.h>

#include "ScanInterface.hpp"

class TApplication;
class TCanvas;
class TFile;
class TTree;
class TH1;
class TH2;
class TH1D;
class TCutG;

extern const double pi;
extern const double cvac;
extern const double Mn;

// Return the neutron TOF for an energy in MeV.
double tof2energy(const double &tof_, const double &d_);

// Return the neutron energy for a TOF in ns.
double energy2tof(const double &E_, const double &d_);

// Add dtheta_ to theta and wrap between 0 and 2pi.
void addAngles(double &theta, const double &dtheta_);

// Return a random number between low and high.
double frand(double low, double high);

// Convert spherical coordinates to cartesian.
void sphere2Cart(const double &r_, const double &theta_, const double &phi_, double &x, double &y, double &z);

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, std::string &left, std::string &right, const char &delim=':');

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, double &left, double &right, const char &delim=':');

// Split a string into two pieces based on a delimiter.
bool splitByColon(const std::string &str, int &left, int &right, const char &delim=':');

// Split a filename into path and extension.
bool splitFilename(const std::string &str, std::string &left, std::string &right, const char &delim='.');

class interpolator{
  public:
	interpolator();

	interpolator(const char *fname_);

	bool load(const char *fname_);

	bool interpolate(const double &x_, double &y);

	double interpolate(const double &x_);

	bool empty(){ return xvals.empty(); }

  private:
	std::vector<double> xvals;
	std::vector<double> yvals;
};

class progressBar{
  public:
	long long numEntries;
	long long length;
	long long chunkSize;
	long long chunkCount;
	std::string progStr;
	
	progressBar(const long long &len=20) : numEntries(0), length(len), chunkSize(0), chunkCount(0), progStr(len, ' '){ }

	void start(const long long &numEntries_);

	void check(const long long &entry_);

	void finalize();
};

class simpleTool{
  protected:
	TApplication *rootapp;
	TCanvas *can1;
	TCanvas *can2;
  
	TFile *infile;
	TFile *outfile;
	
	TTree *intree;
	TTree *outtree;
	
	TCutG *tcutg;

	std::string input_filename;
	std::string input_objname;
	std::string output_filename;
	std::string output_objname;
	std::string cut_filename;
	std::string cut_objname;	

	std::deque<std::string> filename_list;

	std::vector<option> longOpts; /// Vector of all command line options.
	std::vector<optionExt> baseOpts; /// Base level command line options for the scan.
	std::vector<optionExt> userOpts; /// User added command line options.
	std::string optstr;

	std::string full_input_filename;

	std::string work_dir;
	std::string home_dir;

	bool debug;
	bool filledFromTree;

	long long start_entry;
	long long entries_to_process;
	long long max_entries_to_process;
	long long current_entry;

	progressBar pbar;

	virtual void addOptions(){ }
	
	virtual bool processArgs(){ return true; }

	/** Get the specified entry from the input TTree.
	  * \param[in]  entry_ The entry to get from the TTree.
	  * \return True if the TTree is loaded and the specified entry exists and return false otherwise.
	  */
	virtual bool getEntry(const long long &entry_);

	/** Get the next entry from the input TTree.
	  * \return True if the TTree is loaded and the next entry exists and return false otherwise.
	  */
	virtual bool getNextEntry();

	/** Get the full pathname from an input string.
	  * \param[in]  path_ The user specified filename. May be relative or absolute.
	  * \return String containing the full path to the input file.
	  */
	std::string getRealPath(const std::string &path_);

  public:
	simpleTool();
 
	virtual ~simpleTool();

	void help(char *prog_name_);

	bool setup(int argc, char *argv[]);

	TApplication *initRootGraphics();

	TCanvas *openCanvas1(const std::string &title_="Canvas");

	TCanvas *openCanvas2(const std::string &title_="Canvas");
	
	TFile *openInputFile();
	
	TTree *loadInputTree();
	
	TFile *openOutputFile();

	TCutG *loadTCutG();
	
	TCanvas *getCanvas1(){ return can1; }
	
	TCanvas *getCanvas2(){ return can2; }
	
	TFile *getInputFile(){ return infile; }
	
	TFile *getOutputFile(){ return outfile; }
	
	void wait();
};

class simpleHistoFitter : public simpleTool {
  protected:
  	TH1 *h1d;
	TH2 *h2d;

	size_t firstChildOption; /// The index of the first command line option added by a derived class.

	int histStartID;
	int histStopID;

	unsigned int nEntries;

	std::string draw_string;
	std::string expr_string;

	bool useProjX;

	bool getProjectionX(TH1 *h1_, TH2 *h2_, const int &binY_);
	
	bool getProjectionY(TH1 *h1_, TH2 *h2_, const int &binX_);	

	bool getProjection(TH1 *h1_, TH2 *h2_, const int &bin_);

	TH1D *getProjectionHist(TH2 *h2_, const char *name_="h1", const char *title_="");

	int getNumProjections(TH2 *h2_);

	double getBinLowEdge(TH2 *h2_, const int &bin_);
	
	double getBinWidth(TH2 *h2_, const int &bin_);

	double getMaximum(TH1 *h1_, const double &lowVal_, const double &highVal_, double &mean);

	void addOptions();
	
	bool processArgs();

	virtual void addChildOptions(){ }

	virtual bool processChildArgs(){ return true; }
	
  public:
	simpleHistoFitter();
	
	TH2 *fillHistogram();
	
	TH1 *getHist1D(){ return h1d; }
	
	TH2 *getHist2D(){ return h2d; }
	
	int execute(int argc, char *argv[], bool drawh2_=false);
	
	virtual bool process(){ return false; }
};

#endif
