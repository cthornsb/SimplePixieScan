#include "PhoswichProcessor.hpp"
#include "MapFile.hpp"

#include "TTree.h"
#include "TGraph.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"

/// Set the fit parameters for the current event.
bool PhoswichProcessor::SetFitParameters(ChannelEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }

	// Set the initial parameters of the fast pulse.
	unsigned int x1 = event_->max_index - fitting_low;
	unsigned int x2 = event_->max_index + fitting_high;

	fitting_func->SetRange((double)x1, (double)x2);
	fitting_func->SetParameter(0, 5.571827*event_->maximum - 0.9336001); // Constant
	fitting_func->SetParameter(1, event_->max_index); // MPV
	fitting_func->SetParameter(2, 1.65004); // Sigma
	
	// Set the initial parameters of the slow pulse.
	x1 = event_->max_index + fitting_low; 
	x2 = event_->max_index + fitting_high2;
	double y1 = event_->yvals[x1]-fitting_func->Eval(x1); 
	double y2 = event_->yvals[x2]-fitting_func->Eval(x2);
	if(y1 <= 0.0){ y1 = 0.1; }
	if(y2 <= 0.0){ y2 = 0.1; }

	double b = 0.0;
	double A = 0.0;
	if(y2 >= y1){
		b = std::log(y2/y1) / (x2 - x1);
		A = std::log(y2) -b*x2;
	}

	fitting_func2->SetRange((double)x1, (double)x2);
	fitting_func2->SetParameter(0, A);
	fitting_func2->SetParameter(1, b);
	fitting_func2->FixParameter(2, fitting_func->GetParameter(0));
	fitting_func2->FixParameter(3, fitting_func->GetParameter(1));
	fitting_func2->FixParameter(4, fitting_func->GetParameter(2));
	
	return true;
}
	
/// Fit a single trace.
bool PhoswichProcessor::FitPulse(TGraph *trace_, float &phase){
	if(!trace_){ return false; }
	
	// Fit the fast pulse.
	fit_result = trace_->Fit(fitting_func, "SQRE");
	
	fast_A = fitting_func->GetParameter(0);
	fast_MPV = fitting_func->GetParameter(1);
	fast_Sigma = fitting_func->GetParameter(2);
	fast_chi2 = fit_result->Chi2()/fit_result->Ndf();

	// Fit the slow pulse.
	fit_result = trace_->Fit(fitting_func2, "SQRE");

	// f(x) = exp(A + B*x) = C * exp(B*x)
	slow_A = fitting_func2->GetParameter(0); // = ln(C)
	slow_Slope = fitting_func2->GetParameter(1);
	slow_chi2 = fit_result->Chi2()/fit_result->Ndf();

	// Update the phase of the pulse by moving 3 sigma from the MPV.
	phase = fitting_func->GetParameter(1) - 3*fitting_func->GetParameter(2);
	
	return true;
}

/// Process all individual events.
bool PhoswichProcessor::HandleEvents(){
	if(!init){ return false; }

	ChannelEvent *current_event;

	for(std::deque<ChannelEventPair*>::iterator iter = events.begin(); iter != events.end(); iter++){
		current_event = (*iter)->event;
	
		// Check that the time and energy values are valid
		if(!current_event->valid_chan){ continue; }
	
		// Fill the values into the root tree.
		structure.Append(current_event->hires_time, current_event->hires_energy, fast_A, fast_MPV, fast_Sigma, fast_chi2, slow_A, slow_Slope, slow_chi2);
		
		// Copy the trace to the output file.
		if(write_waveform){
			waveform.Append((int*)current_event->yvals, current_event->size);
		}
		
		good_events++;
	}
	return true;
}

PhoswichProcessor::PhoswichProcessor(MapFile *map_) : Processor("Phoswich", "phoswich", map_){
	fitting_func = new TF1("f_fast", "landau", 0, 1);
	fitting_func2 = new TF1("f_slow", "expo(0)+landau(2)", 0, 1);
	fitting_low = 5;
	fitting_high = 8;
	fitting_low2 = 15;
	fitting_high2 = 100;
}

PhoswichProcessor::~PhoswichProcessor(){
	// fitting_func is deleted by Processor::~Processor()
	if(fitting_func2){ delete fitting_func2; }
}

bool PhoswichProcessor::Initialize(TTree *tree_){
	if(init || !tree_){ 
		PrintMsg("Root output is already initialized!");
		return false; 
	}
	
	// Add a branch to the tree
	PrintMsg("Adding branch to main TTree.");
	local_branch = tree_->Branch(name.c_str(), &structure);
	
	if(write_waveform){
		PrintMsg("Writing raw waveforms to file.");
		local_branch = tree_->Branch((name+"Wave").c_str(), &waveform);
	}
	
	return (init = true);
}

void PhoswichProcessor::Zero(){
	structure.Zero();
	waveform.Zero();
}
