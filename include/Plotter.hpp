#ifndef PLOTTER_HPP
#define PLOTTER_HPP

#include <string>

class TH1;
class TPad;

class Plotter{
  private:
	TH1 *hist;
	
	int dim;

	double xmin, xmax;
	double ymin, ymax;
	
	std::string name;
	std::string opt;	
	
  public:
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_,
	        const std::string &xtitle_, const int &xbins_, const double &xmin_, const double &xmax_);
	
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_,
	        const std::string &xtitle_, const int &xbins_, const double &xmin_, const double &xmax_,
	        const std::string &ytitle_, const int &ybins_, const double &ymin_, const double &ymax_);
	
	~Plotter();
	
	TH1 *GetHist(){ return hist; }
	
	int GetNdim(){ return dim; }
	
	std::string GetName(){ return name; }
	
	std::string GetDrawOption(){ return opt; }
	
	void SetXaxisTitle(const std::string &title_);
	
	void SetYaxisTitle(const std::string &title_);
	
	void SetStats(const bool &state_=true);
	
	void SetXrange(const double &xmin_, const double &xmax_);
	
	void SetYrange(const double &ymin_, const double &ymax_);
	
	void SetRange(const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_);
	
	void Zero();
	
	void Fill(const double &x_);
	
	void Fill(const double &x_, const double &y_);
	
	void Draw(TPad *pad_);
};

#endif
