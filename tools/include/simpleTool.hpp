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

class progressBar{
  public:
	unsigned int numEntries;
	unsigned int length;
	unsigned int chunkSize;
	unsigned int chunkCount;
	std::string progStr;
	
	progressBar(const unsigned int &len=20) : numEntries(0), length(len), chunkSize(0), chunkCount(0), progStr(len, ' '){ }

	void start(const unsigned int &numEntries_);

	void check(const unsigned int &entry_);

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
	
	virtual void addOptions(){ }
	
	virtual bool processArgs(){ return true; }

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

	std::string draw_string;

	bool debug;
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
	
	int execute(int argc, char *argv[]);
	
	virtual bool process(){ return false; }
};

#endif
