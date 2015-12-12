#ifndef ONLINE_PROCESSOR_HPP
#define ONLINE_PROCESSOR_HPP

class TH1;
class TH2;
class TCanvas;
class TApplication;

class OnlineProcessor{
  private:
	unsigned int which_hists[4];

	bool init;

	TH1* hists_1d[5]; /// Array of 1d histogram pointers for plotting online data.
	TH2* hists_2d[5]; /// Array of 2d histogram pointers for plotting online data.

	TCanvas *can; /// Root canvas for plotting online data.
	
	TApplication *rootapp; /// Root application for handling graphics.
	
	/// Redraw all current plots to the canvas.
	void RedrawRoot();
	
  public:
	OnlineProcessor();
	
	~OnlineProcessor();
	
	/// Initialize.
	bool Initialize();
	
	/// Set the range of a 1d histogram.
	void SetRange(const unsigned int &index_, const double &low_, const double &high_);
	
	/// Set the range of a 2d histogram.
	void SetRange(const unsigned int &index_, const double &Xlow_, const double &Xhigh_, const double &Ylow_, const double &Yhigh_);
	
	/// Refresh online plots.
	void Refresh();
};

#endif
