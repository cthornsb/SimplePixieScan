std::ofstream ofile;

void processSpecFile(const char *fname, const int &finalBin=-1, const double &coeff=0.7){
	TNamed *named;
	TF1 *func, *peak;

	TFile *f = new TFile(fname, "READ");

	if(!f || !f->IsOpen()) return;

	std::cout << "Dir\tA\tEdge\tMean\n";

	int count = 1;
	double amplitude;
	while(true){
		std::stringstream stream;
		stream << "bin";
		if(count < 10) stream << "00";
		else if(count < 100) stream << "0";
		stream << count++;
		if(finalBin > 0){
			if(count > finalBin) break;
		}
		else if(!f->cd(stream.str().c_str())) break;
		f->GetObject((stream.str()+"/amplitude").c_str(), named);
		f->GetObject((stream.str()+"/func").c_str(), func);
		f->GetObject((stream.str()+"/peakfunc").c_str(), peak);
		if(named && func && peak){
			amplitude = strtod(named->GetTitle(), NULL);
			std::cout << stream.str() << "\t" << amplitude << "\t" << func->GetX(amplitude*coeff) << "\t" << peak->GetParameter(1) << std::endl;
		}
	}

	f->Close();
	delete f;
}

void readCounts(const char *fname){
	const std::string dirs[4] = {"counts/trace/", "counts/liquid/", "counts/logic/", "counts/genericbar/"};

	TFile *f = new TFile(fname, "READ");

	if(!f || !f->IsOpen()) return;

	ofile << fname << "\t";

	TNamed *named;
	unsigned long long total, handled, unhandled;
	for(int i = 0; i < 4; i++){
		f->GetObject((dirs[i]+"Total").c_str(), named);
		if(!named) continue;
		total = strtoull(named->GetTitle(), NULL, 10);
		f->GetObject((dirs[i]+"Unhandled").c_str(), named);
		unhandled = strtoull(named->GetTitle(), NULL, 10);

		ofile << total << "\t" << unhandled << "\t";
	}

	ofile << "\n";

	f->Close();
	delete f;
}

void readFile(const char *fname){
	TFile *f = new TFile(fname, "READ");

	ofile << fname << "\t";

	TNamed *named;
	f->GetObject("head/file01/Data time", named);
	ofile << named->GetTitle() << "\t";

	f->GetObject("counts/logic/Handled", named);
	ofile << named->GetTitle() << "\n";

	f->Close();
	delete f;
}

int reader(){
	return 0;
}
