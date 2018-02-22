
/** Returns a color on a smooth spectrum (magenta to red).
  * \param[in]  val_: The current value to convert to a color.
  * \param[in]  max_: The maximum value in the data set.
  * \param[out] color_: The root TColor representing the smoothed spectrum color.
  * \return Nothing
  */
void SmoothSpectrum(const double &val_, const double &max_, TColor &color, bool invert=false){
        int code = int((val_/max_)*1195);
        if(code == 0)
		color.SetRGB(0.6, 0.8, 1.0);
        else if(code < 175)
		color.SetRGB((175-code)/255.0, 0, 1.0);
        else if(code < 430)
		color.SetRGB(0, (code-175)/255.0 , 1.0);
        else if(code < 685)
		color.SetRGB(0, 1.0, 1.0-(code-430)/255.0);
        else if(code < 940)
		color.SetRGB((code-685)/255.0, 1.0, 0);
        else if(code < 1195)
		color.SetRGB(1.0, 1.0-(code-940)/255.0,0);
        else
		color.SetRGB(1.0, 0, 0);

	// Invert the color spectrum.
	if(invert) color.SetRGB(1-color.GetRed(), 1-color.GetGreen(), 1-color.GetBlue());
}

void CreateColorSpectrum(const size_t &N, const TColor &color1, const TColor &color2, std::vector<Color_t> &colors, bool invert=false){
	colors.clear();
	double stepSize[3] = {(color2.GetRed()-color1.GetRed())/(N-1), (color2.GetGreen()-color1.GetGreen())/(N-1), (color2.GetBlue()-color1.GetBlue())/(N-1)};
	TColor tempColor;
	for(size_t i = 0; i < N; i++){
		if(!invert)
			tempColor.SetRGB(color1.GetRed()+stepSize[0]*i, color1.GetGreen()+stepSize[1]*i, color1.GetBlue()+stepSize[2]*i);
		else
			tempColor.SetRGB(1-(color1.GetRed()+stepSize[0]*i), 1-(color1.GetGreen()+stepSize[1]*i), 1-(color1.GetBlue()+stepSize[2]*i));
		colors.push_back(TColor::GetColor(tempColor.GetRed(), tempColor.GetGreen(), tempColor.GetBlue()));
	}
}

void CreateColorSpectrum(const size_t &N, std::vector<Color_t> &colors, bool invert=false){
	colors.clear();
	TColor tempColor;
	size_t numColors = (N >= 2 ? N : 2);
	double stepSize = 1.0/(N-1);
	for(size_t i = 0; i < numColors; i++){
		SmoothSpectrum(i*stepSize, 1.0, tempColor, invert);
		colors.push_back(TColor::GetColor(tempColor.GetRed(), tempColor.GetGreen(), tempColor.GetBlue()));
	}
}

class TGraph3D{
  public:
	TGraph3D() : isModified(true), drawLine(true), drawMarkers(true), nPts(0), fX(NULL), fY(NULL), fZ(NULL), goodPoint(NULL), poly(NULL), marker(NULL), xaxis("x"), yaxis("y"), zaxis("z") { }
	
	TGraph3D(const size_t &N) : isModified(true), drawLine(true), drawMarkers(true), nPts(N), fX(NULL), fY(NULL), fZ(NULL), goodPoint(NULL), poly(NULL), marker(NULL), xaxis("x"), yaxis("y"), zaxis("z"), fMarkerPts(N, 0) { 
		fX = new double[nPts];
		fY = new double[nPts];
		fZ = new double[nPts];
		goodPoint = new bool[nPts];
		for(size_t i = 0; i < nPts; i++){
			fX[i] = 0; 
			fY[i] = 0; 
			fZ[i] = 0;
			goodPoint[i] = false;
		}
		poly = new TPolyLine3D(nPts, fMarkerPts.data());
		marker = new TPolyMarker3D(nPts, fMarkerPts.data());
	}
	
	TGraph3D(const size_t &N, double *x, double *y, double *z) : isModified(true), drawLine(true), drawMarkers(true), nPts(N), fX(x), fY(y), fZ(z), goodPoint(NULL), poly(NULL), marker(NULL), xaxis("x"), yaxis("y"), zaxis("z"), fMarkerPts(N, 0) { 
		goodPoint = new bool[nPts];
		for(int i = 0; i < nPts; i++)
			goodPoint[i] = true;
		poly = new TPolyLine3D(nPts, fMarkerPts.data());
		marker = new TPolyMarker3D(nPts, fMarkerPts.data());
	}

	~TGraph3D(){
		delete[] fX;
		delete[] fY;
		delete[] fZ;
		delete[] goodPoint;
		delete poly;
		delete marker;
	}

	void SetMarkerColor(Color_t mcolor_){ mcolor = mcolor_; }
	
	void SetMarkerSize(Size_t msize_){ msize = msize_; }
	
	void SetMarkerStyle(Style_t mstyle_){ mstyle = mstyle_; }

	void SetLineColor(Color_t lcolor){ poly->SetLineColor(lcolor); }
	
	void SetLineWidth(Size_t lwidth){ poly->SetLineWidth(lwidth); }
	
	void SetLineStyle(Style_t lstyle){ poly->SetLineStyle(lstyle); }

	bool ToggleDrawLine(){ return (drawLine = !drawLine); }

	bool ToggleDrawMarkers(){ return (drawMarkers = !drawMarkers); }

	bool SetAxes(const std::string &axes){
		double *ptrs[3] = {fX, fY, fZ};
		double *newPtrs[3] = {NULL, NULL, NULL};
		switch(axes[0]){
			case 'x':
				newPtrs[0] = ptrs[0];
				break;
			case 'y':
				newPtrs[0] = ptrs[1];
				xaxis = "y";
				break;
			case 'z':
				newPtrs[0] = ptrs[2];
				xaxis = "z";
				break;
			default:
				std::cout << " X: Invalid axis (" << axes[0] << ")!\n";
				return false;
		}
		switch(axes[1]){
			case 'x':
				newPtrs[1] = ptrs[0];
				yaxis = "x";
				break;
			case 'y':
				newPtrs[1] = ptrs[1];
				break;
			case 'z':
				newPtrs[1] = ptrs[2];
				yaxis = "z";
				break;
			default:
				std::cout << " Y: Invalid axis (" << axes[0] << ")!\n";
				return false;
		}
		switch(axes[2]){
			case 'x':
				newPtrs[2] = ptrs[0];
				zaxis = "x";
				break;
			case 'y':
				newPtrs[2] = ptrs[1];
				zaxis = "y";
				break;
			case 'z':
				newPtrs[2] = ptrs[2];
				break;
			default:
				std::cout << " Z: Invalid axis (" << axes[0] << ")!\n";
				return false;
		}
		if(!newPtrs[0] || !newPtrs[1] || !newPtrs[2]) 
			return false;
		fX = newPtrs[0];
		fY = newPtrs[1];
		fZ = newPtrs[2];
		isModified = true;
		return true;
	}

	void SetPoint(const int &i, const double &x, const double &y, const double &z){
		fX[i] = x;
		fY[i] = y;
		fZ[i] = z;
		goodPoint[i] = true;
		isModified = true;
	}
	
	void SetPoints(const size_t &index, const size_t &N, double *x, double *y, const double &z){
		memcpy((char*)&fX[index], (char*)x, N*sizeof(double));
		memcpy((char*)&fY[index], (char*)y, N*sizeof(double));
		for(size_t i = index; i < index+N; i++){
			goodPoint[i] = true;
			fZ[i] = z;
		}
		isModified = true;
	}

	void GetLimitsX(double &xMin, double &xMax){
		xMin = std::numeric_limits<double>::max();
		xMax = std::numeric_limits<double>::min();
		for(int i = 0; i < nPts; i++){
			if(!goodPoint[i]) continue;
			if(fX[i] < xMin) xMin = fX[i];
			if(fX[i] > xMax) xMax = fX[i];
		}
	}

	void GetLimitsY(double &yMin, double &yMax){
		yMin = std::numeric_limits<double>::max();
		yMax = std::numeric_limits<double>::min();
		for(int i = 0; i < nPts; i++){
			if(!goodPoint[i]) continue;
			if(fY[i] < yMin) yMin = fY[i];
			if(fY[i] > yMax) yMax = fY[i];
		}
	}

	void GetLimitsZ(double &zMin, double &zMax){
		zMin = std::numeric_limits<double>::max();
		zMax = std::numeric_limits<double>::min();
		for(int i = 0; i < nPts; i++){
			if(!goodPoint[i]) continue;
			if(fZ[i] < zMin) zMin = fZ[i];
			if(fZ[i] > zMax) zMax = fZ[i];
		}
	}

	bool GetIsModified(){ return isModified; }

	void Draw3D(const char *opt_=""){
		size_t nPtsToDraw;
		if(isModified){
			fMarkerPts.clear();
			for(size_t i = 0; i < nPts; i++){
				if(!goodPoint[i]) continue;
				fMarkerPts.push_back((float)fX[i]);
				fMarkerPts.push_back((float)fY[i]);
				fMarkerPts.push_back((float)fZ[i]);
			}
			isModified = false;
			nPtsToDraw = fMarkerPts.size()/3;
			poly->SetPolyLine(nPtsToDraw, fMarkerPts.data());
			marker->SetPolyMarker(nPtsToDraw, fMarkerPts.data(), mstyle);
			marker->SetMarkerColor(mcolor);
			marker->SetMarkerSize(msize);
		}
		if(drawLine)
			poly->DrawPolyLine(0, fMarkerPts.data(), opt_);
		if(drawMarkers)
			marker->DrawPolyMarker(0, fMarkerPts.data(), mstyle, opt_);
	}

  private:
	bool isModified;
	bool drawLine;
	bool drawMarkers;

	size_t nPts;

	double *fX, *fY, *fZ;
	bool *goodPoint;
	
	TPolyLine3D *poly;
	TPolyMarker3D *marker;

	const char *xaxis;
	const char *yaxis;
	const char *zaxis;

	Color_t	mcolor;
	Size_t msize;
	Style_t mstyle;

	std::vector<float> fMarkerPts;
};

class TGraphGroup3D{
  public:
	TGraphGroup3D(const size_t &nGraphs_, const size_t &graphLength_) : nGraphs(nGraphs_), graphLength(graphLength_), reverseDrawOrder(false), graphs(NULL), h3(NULL), xAxisLabel("x"), yAxisLabel("y"), zAxisLabel("z") {
		graphs = new TGraph3D*[nGraphs];
		for(size_t i = 0; i < nGraphs; i++)
			graphs[i] = new TGraph3D(graphLength);
	}

	~TGraphGroup3D(){
		for(size_t i = 0; i < nGraphs; i++)
			delete graphs[i];
		delete[] graphs;
	}

	///////////////////////////////////////////////////////////////////////////////

	void SetMarkerColor(const size_t &graph_, Color_t mcolor){ 
		if(graph_ < nGraphs) graphs[graph_]->SetMarkerColor(mcolor);
	}
	
	void SetMarkerSize(const size_t &graph_, Size_t msize){ 
		if(graph_ < nGraphs) graphs[graph_]->SetMarkerSize(msize);
	}
	
	void SetMarkerStyle(const size_t &graph_, Style_t mstyle){ 
		if(graph_ < nGraphs) graphs[graph_]->SetMarkerStyle(mstyle);
	}

	void SetLineColor(const size_t &graph_, Color_t lcolor){ 
		if(graph_ < nGraphs) graphs[graph_]->SetLineColor(lcolor);
	}
	
	void SetLineWidth(const size_t &graph_, Size_t lwidth){ 
		if(graph_ < nGraphs) graphs[graph_]->SetLineWidth(lwidth);
	}
	
	void SetLineStyle(const size_t &graph_, Style_t lstyle){ 
		if(graph_ < nGraphs) graphs[graph_]->SetLineStyle(lstyle);
	}

	void ToggleDrawLine(const size_t &graph_){ 
		if(graph_ < nGraphs) 
			graphs[graph_]->ToggleDrawLine();
	}

	void ToggleDrawMarkers(const size_t &graph_){ 
		if(graph_ < nGraphs) 
			graphs[graph_]->ToggleDrawMarkers();
	}

	///////////////////////////////////////////////////////////////////////////////

	void SetMarkerColor(Color_t mcolor){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetMarkerColor(mcolor);
	}
	
	void SetMarkerSize(Size_t msize){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetMarkerSize(msize);
	}
	
	void SetMarkerStyle(Style_t mstyle){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetMarkerStyle(mstyle);
	}

	void SetLineColor(Color_t lcolor){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetLineColor(lcolor);
	}
	
	void SetLineWidth(Size_t lwidth){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetLineWidth(lwidth);
	}
	
	void SetLineStyle(Style_t lstyle){ 
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetLineStyle(lstyle);
	}

	void ToggleDrawLine(){ 
		for(size_t i = 0; i < nGraphs; i++) 
			graphs[i]->ToggleDrawLine();
	}

	void ToggleDrawMarkers(){ 
		for(size_t i = 0; i < nGraphs; i++) 
			graphs[i]->ToggleDrawMarkers();
	}

	///////////////////////////////////////////////////////////////////////////////

	void SetXaxisLabel(const std::string &label_){ xAxisLabel = label_; }

	void SetYaxisLabel(const std::string &label_){ yAxisLabel = label_; }

	void SetZaxisLabel(const std::string &label_){ zAxisLabel = label_; }

	bool ToggleReverseDrawOrder(){ return (reverseDrawOrder = !reverseDrawOrder); }

	bool SetAxes(const std::string &axes){ 
		if(axes.length() < 3) return false;
		for(size_t i = 0; i < nGraphs; i++) 
			graphs[i]->SetAxes(axes);
		return true;
	}

	bool ReadTTree(TTree *tree_, const char *xname_, const char *yname_, const char *zname_){
		double values[3];
		TBranch *branches[3] = {NULL, NULL, NULL};
		tree_->SetMakeClass(1);
		tree_->SetBranchAddress(xname_, &values[0], &branches[0]);
		tree_->SetBranchAddress(yname_, &values[1], &branches[1]);
		tree_->SetBranchAddress(zname_, &values[2], &branches[2]);

		if(!branches[0]){
			std::cout << " Error! Failed to read from X-axis branch named \"" << xname_ << "\".\n";
			return false;
		}
		if(!branches[1]){
			std::cout << " Error! Failed to read from Y-axis branch named \"" << yname_ << "\".\n";
			return false;
		}
		if(!branches[2]){
			std::cout << " Error! Failed to read from Z-axis branch named \"" << zname_ << "\".\n";
			return false;
		}

		long long maxEntry = (nGraphs*graphLength < tree_->GetEntries() ? nGraphs*graphLength : tree_->GetEntries());
		size_t currentGraph = 0;
		size_t currentIndex = 0;

		for(long long entry = 0; entry < maxEntry; entry++){
			tree_->GetEntry(entry);
			if(values[2] != 0)
				graphs[currentGraph]->SetPoint(currentIndex++, values[0], values[1], values[2]);
			if((entry + 1) % graphLength == 0){
				currentGraph++;
				currentIndex = 0;
			}
		}

		return true;
	}

	void SetPoint(const size_t &graph_, const int &i, const double &x, const double &y, const double &z){
		if(graph_ < nGraphs) graphs[graph_]->SetPoint(i, x, y, z);
	}
	
	void SetPoints(const size_t &graph_, const size_t &index, const size_t &N, double *x, double *y, const double &z){
		if(graph_ < nGraphs) graphs[graph_]->SetPoints(index, N, x, y, z);
	}

	void SetPoint(const int &i, const double &x, const double &y, const double &z){
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetPoint(i, x, y, z);
	}
	
	void SetPoints(const size_t &index, const size_t &N, double *x, double *y, const double &z){
		for(size_t i = 0; i < nGraphs; i++) graphs[i]->SetPoints(index, N, x, y, z);
	}

	void Draw3D(){
		SetupAxes3D();
		h3->Draw();
		if(!reverseDrawOrder){
			for(size_t i = 0; i < nGraphs; i++)
				graphs[i]->Draw3D("SAME");
		}
		else{
			for(size_t i = nGraphs; i > 0; i--)
				graphs[i-1]->Draw3D("SAME");
		}
	}

  private:
	size_t nGraphs;
	size_t graphLength;

	bool reverseDrawOrder;

	TGraph3D **graphs;

	TH3F *h3;

	std::string xAxisLabel;
	std::string yAxisLabel;
	std::string zAxisLabel;
	
	double xMinOverall, xMaxOverall;
	double yMinOverall, yMaxOverall;
	double zMinOverall, zMaxOverall;

	void SetupAxes3D(){
		xMinOverall = std::numeric_limits<double>::max(); xMaxOverall = std::numeric_limits<double>::min();
		yMinOverall = std::numeric_limits<double>::max(); yMaxOverall = std::numeric_limits<double>::min();
		zMinOverall = std::numeric_limits<double>::max(); zMaxOverall = std::numeric_limits<double>::min();

		double xmin, xmax;
		double ymin, ymax;
		double zmin, zmax;

		bool anyModified = false;
		for(size_t i = 0; i < nGraphs; i++){
			if(!graphs[i]->GetIsModified()) continue;
			graphs[i]->GetLimitsX(xmin, xmax);
			graphs[i]->GetLimitsY(ymin, ymax);
			graphs[i]->GetLimitsZ(zmin, zmax);
			if(xmin < xMinOverall) xMinOverall = xmin;
			if(xmax > xMaxOverall) xMaxOverall = xmax;
			if(ymin < yMinOverall) yMinOverall = ymin;
			if(ymax > yMaxOverall) yMaxOverall = ymax;
			if(zmin < zMinOverall) zMinOverall = zmin;
			if(zmax > zMaxOverall) zMaxOverall = zmax;
			anyModified = true;
		}

		if(anyModified){
			if(h3) delete h3;
			h3 = new TH3F("h3", "TGraph3D", 100, xMinOverall-std::fabs(xMinOverall)*0.1, xMaxOverall+std::fabs(xMaxOverall)*0.1, 
				                        100, yMinOverall-std::fabs(yMinOverall)*0.1, yMaxOverall+std::fabs(yMaxOverall)*0.1, 
				                        100, zMinOverall-std::fabs(zMinOverall)*0.1, zMaxOverall+std::fabs(zMaxOverall)*0.1);
			h3->SetStats(0);
			h3->GetXaxis()->SetTitle(xAxisLabel.c_str());
			h3->GetYaxis()->SetTitle(yAxisLabel.c_str());
			h3->GetZaxis()->SetTitle(zAxisLabel.c_str());
		}
	}
};

TGraphGroup3D *g1;

int TestTGraph3D(){
	double xpts[100];
	double ypts[100];
	
	double width = 2*3.14159/100;
	for(int i = 0; i < 100; i++){
		xpts[i] = -3.14159+width*i;
		ypts[i] = std::sin(xpts[i]);
	}

	g1 = new TGraphGroup3D(10, 100);
	
	for(int i = 0; i < 10; i++){
		for(int j = 0; j < 100; j++){
			g1->SetPoint(i, j, xpts[j], 0.1*i*ypts[j], i);
		}
	}

	if(!g1->SetAxes("xzy")) return -1;
	
	g1->SetLineColor(kGreen+3);
	g1->SetLineWidth(2);
	//g1->SetMarkerColor(kGreen+3);
	//g1->SetMarkerStyle(20);	
	
	g1->Draw3D();
	gPad->Update();

	return 0;
}
