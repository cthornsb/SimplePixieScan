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

	bool logx;
	bool logy;
	bool logz;
	
	std::string name;
	std::string opt;	

  public:
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_, 
	        const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_);
	
	Plotter(const std::string &name_, const std::string &title_, const std::string &draw_opt_, const std::string &xtitle_,
	        const std::string &xunits_, const int &xbins_, const double &xmin_, const double &xmax_, const std::string &ytitle_,
	        const std::string &yunits_, const int &ybins_, const double &ymin_, const double &ymax_);
	
	~Plotter();
	
	TH1 *GetHist(){ return hist; }
	
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
	
	void SetXaxisTitle(const std::string &title_);
	
	void SetYaxisTitle(const std::string &title_);
	
	void SetStats(const bool &state_=true);
	
	void SetXrange(const double &xmin_, const double &xmax_);
	
	void SetYrange(const double &ymin_, const double &ymax_);
	
	void SetRange(const double &xmin_, const double &xmax_, const double &ymin_, const double &ymax_);
	
	void ResetXrange();
	
	void ResetYrange();
	
	void ResetRange();
	
	void ToggleLogX(){ logx = !logx; }
	
	void ToggleLogY(){ logy = !logy; }
	
	void ToggleLogZ(){ logz = !logz; }
	
	void Zero();
	
	void Fill(const double &x_, const double &y_=0.0);
	
	void Draw(TPad *pad_);
	
	void Write();
};

#endif
