#include "PhoswichProcessor.hpp"
#include "MapFile.hpp"
#include "Plotter.hpp"

#include "TGraph.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"

/// Set the fit parameters for the current event.
/*bool PhoswichProcessor::SetFitParameters(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }

	// Set the initial parameters of the fast pulse.
	fast_x1 = event_->max_index - fitting_low;
	fast_x2 = event_->max_index + fitting_high;

	fitting_func->SetRange((double)fast_x1, (double)fast_x2);
	fitting_func->SetParameter(0, 5.571827*event_->maximum - 0.9336001); // Constant
	fitting_func->SetParameter(1, event_->max_index); // MPV
	fitting_func->SetParameter(2, 1.65004); // Sigma
	
	// Compute the trace qdc of the slow component of the pulse.
	slow_qdc = event_->IntegratePulse(event_->max_index + fitting_low2, event_->max_index + fitting_high2);
	
	return true;
}*/
	
/// Fit a single trace.
/*bool PhoswichProcessor::FitPulse(TGraph *trace_, float &phase){
	if(!trace_){ return false; }
	
	// Fit the fast pulse.
	trace_->Fit(fitting_func, "QRE");
	
	fast_A = fitting_func->GetParameter(0);
	fast_MPV = fitting_func->GetParameter(1);
	fast_Sigma = fitting_func->GetParameter(2);
	fast_chi2 = fitting_func->GetChisquare()/fitting_func->GetNDF();
	fast_qdc = fitting_func->Integral(fast_x1, fast_x2);

	// Compute the phase by subtracting the pulse HWHM from the most-probable-value.
	phase = fitting_func->GetParameter(1) - 1.17741*fitting_func->GetParameter(2);
	
	return true;
}*/

/// Set the CFD parameters for the current event.
bool PhoswichProcessor::SetCfdParameters(ChanEvent *event_, MapEntry *entry_){
	if(!event_ || !entry_){ return false; }
	
	// Compute the trace qdc of the fast and slow component of the pulse.
	fast_qdc = event_->IntegratePulse(event_->max_index - fitting_low, event_->max_index + fitting_high);
	slow_qdc = event_->IntegratePulse(event_->max_index + fitting_low2, event_->max_index + fitting_high2);	
	
	return true;
}

/// Process all individual events.
bool PhoswichProcessor::HandleEvent(ChannelEventPair *chEvt, ChannelEventPair *chEvtR/*=NULL*/){
	ChanEvent *current_event = chEvt->channelEvent;

	if(histsEnabled){
		// Get the location of this detector.
		int location = chEvt->entry->location;

		// Fill all diagnostic histograms.
		fast_energy_1d->Fill(location, fast_qdc);
		slow_energy_1d->Fill(location, slow_qdc);
		energy_2d->Fill2d(location, slow_qdc, fast_qdc);
		phase_1d->Fill(location, current_event->phase); 
	}
	
	// Fill the values into the root tree.
	//if(analyzer == FIT){ structure.Append(current_event->time, fast_MPV, fast_qdc, slow_qdc, fast_A); }
	//else{ structure.Append(current_event->time, current_event->phase, fast_qdc, slow_qdc, current_event->maximum); }
	structure.Append(current_event->time, current_event->phase, fast_qdc, slow_qdc, current_event->maximum); 
	
	return true;
}

PhoswichProcessor::PhoswichProcessor(MapFile *map_) : Processor("Phoswich", "phoswich", map_){
	//fitting_func = new TF1("f_fast", "landau", 0, 1);
	
	fitting_low = 5;
	fitting_high = 5;
	fitting_low2 = 5;
	fitting_high2 = 20;
	
	root_structure = (Structure*)&structure;
	root_waveform = &waveform;
}

void PhoswichProcessor::GetHists(OnlineProcessor *online_){
	if(histsEnabled) return;

	online_->GenerateHist(fast_energy_1d);
	online_->GenerateHist(slow_energy_1d);
	online_->GenerateHist(energy_2d);
	online_->GenerateHist(phase_1d);

	histsEnabled = true;
}
