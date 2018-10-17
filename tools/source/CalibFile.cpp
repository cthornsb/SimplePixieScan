#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>

#include "TFile.h"
#include "TObjString.h"

#include "CalibFile.hpp"

#include "XiaData.hpp"
#include "ScanInterface.hpp"

const double deg2rad = 0.0174532925;
const double rad2deg = 57.295779579;

CalibEntry dummyCalib(NULL, NULL, NULL, NULL);

void PositionCal::Transform(Vector3 &vec){
	rotMatrix.Transform(vec);
}

unsigned int PositionCal::ReadPars(const std::vector<std::string> &pars_){ 
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else if(index == 1) r0 = strtod(iter->c_str(), NULL);
		else if(index == 2) theta = strtod(iter->c_str(), NULL)*deg2rad;
		else if(index == 3) phi = strtod(iter->c_str(), NULL)*deg2rad;
		else if(index == 4) rotTheta = strtod(iter->c_str(), NULL)*deg2rad;
		else if(index == 5) rotPhi = strtod(iter->c_str(), NULL)*deg2rad;
		else if(index == 6) rotPsi = strtod(iter->c_str(), NULL)*deg2rad;
		index++;
	}
	Sphere2Cart(Vector3(r0, theta, phi), position);
	rotMatrix.SetRotationMatrix(rotTheta, rotPhi, rotPsi);
	return id;
}

std::string PositionCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", r0=" << r0 << ", theta=" << theta << ", phi=" << phi << ", rotation=(" << rotTheta << ", " << rotPhi << ", " << rotPsi << ")";
	else output << id << "\t" << r0 << "\t" << theta << "\t" << rotTheta << "\t" << rotPhi << "\t" << rotPsi;
	return output.str();
}

unsigned int TimeCal::ReadPars(const std::vector<std::string> &pars_){ 
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else if(index == 1) t0 = strtod(iter->c_str(), NULL);
		index++;
	}
	return id;
}

bool TimeCal::GetCalTime(double &T){
	T = T - t0;
	return true;
}

double TimeCal::GetCalTime(const double &time_){
	return (time_ - t0);
}

std::string TimeCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", t0=" << t0;
	else output << id << "\t" << t0;
	return output.str();
}

unsigned int EnergyCal::ReadPars(const std::vector<std::string> &pars_){ 
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else vals.push_back(strtod(iter->c_str(), NULL));
		index++;
	}
	return id;
}

bool EnergyCal::GetCalEnergy(const double &adc_, double &E){
	if(vals.empty()) return false;
	E = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		E += vals.at(i)*std::pow(adc_, i);
	return true;
}

double EnergyCal::GetCalEnergy(const double &adc_){
	if(vals.empty()) return adc_;
	double output = 0.0;
	for(size_t i = 0; i < vals.size(); i++)
		output += vals.at(i)*std::pow(adc_, i);
	return output;
}

std::string EnergyCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	int index = 0;
	if(fancy) output << " id=" << id;
	else output << id;
	if(!vals.empty()){
		for(std::vector<double>::iterator iter = vals.begin(); iter != vals.end(); iter++){
			if(fancy) output << ", p" << index++ << "=" << (*iter);
			else output << "\t" << (*iter);
		}
	}
	else if(fancy){ output << ", EMPTY"; }
	return output.str();
}

std::string BarCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", t0=" << t0 << ", beta=" << beta << ", cbar=" << cbar << ", length=" << length << ", width=" << width;
	else output << id << "\t" << t0 << "\t" << beta << "\t" << cbar << "\t" << length << "\t" << width;
	return output.str();
}

unsigned int BarCal::ReadPars(const std::vector<std::string> &pars_){
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = GetPixieID(*iter);
		else if(index == 1) t0 = strtod(iter->c_str(), NULL);
		else if(index == 2) beta = strtod(iter->c_str(), NULL);
		else if(index == 3) cbar = strtod(iter->c_str(), NULL);
		else if(index == 4) length = strtod(iter->c_str(), NULL);
		else if(index == 5) width = strtod(iter->c_str(), NULL);
		index++;
	}
	return id;
}

CalibFile::CalibFile(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_){ 
	Load(timeFilename_, energyFilename_, positionFilename_);
}

bool CalibFile::LoadTimeCal(const char *filename_){
	return LoadCalibFile(filename_, time_calib);
}

bool CalibFile::LoadEnergyCal(const char *filename_){
	return LoadCalibFile(filename_, energy_calib);
}

bool CalibFile::LoadPositionCal(const char *filename_){
	return LoadCalibFile(filename_, position_calib);
}

bool CalibFile::LoadBarCal(const char *filename_){
	return LoadCalibFile(filename_, bar_calib);
}

bool CalibFile::Load(const char *timeFilename_, const char *energyFilename_, const char *positionFilename_){
	return (LoadTimeCal(timeFilename_) && LoadEnergyCal(energyFilename_) && LoadPositionCal(positionFilename_));
}

TimeCal *CalibFile::GetTimeCal(const unsigned int &id_){
	if(id_ >= time_calib.size()){ return NULL; }
	return &time_calib.at(id_);
}

TimeCal *CalibFile::GetTimeCal(XiaData *event_){
	return GetTimeCal(16*event_->modNum+event_->chanNum); 
}

EnergyCal *CalibFile::GetEnergyCal(const unsigned int &id_){
	if(id_ >= energy_calib.size()){ return NULL; }
	return &energy_calib.at(id_);
}

EnergyCal *CalibFile::GetEnergyCal(XiaData *event_){
	return GetEnergyCal(16*event_->modNum+event_->chanNum); 
}

PositionCal *CalibFile::GetPositionCal(const unsigned int &id_){
	if(id_ >= position_calib.size()){ return NULL; }
	return &position_calib.at(id_);
}

PositionCal *CalibFile::GetPositionCal(XiaData *event_){
	return GetPositionCal(16*event_->modNum+event_->chanNum); 
}

BarCal *CalibFile::GetBarCal(const unsigned int &id_){
	if(id_ >= bar_calib.size()){ return NULL; }
	return &bar_calib.at(id_);
}

BarCal *CalibFile::GetBarCal(XiaData *event_){
	return GetBarCal(16*event_->modNum+event_->chanNum); 
}

CalibEntry *CalibFile::GetCalibEntry(const unsigned int &id_){
	TimeCal *tcal = this->GetTimeCal(id_);
	EnergyCal *ecal = this->GetEnergyCal(id_);
	PositionCal *pcal = this->GetPositionCal(id_);
	BarCal *bcal = this->GetBarCal(id_);
	return (new CalibEntry(tcal, ecal, pcal, bcal));
}

CalibEntry *CalibFile::GetCalibEntry(XiaData *event_){
	return GetCalibEntry(16*event_->modNum+event_->chanNum); 
}

void CalibFile::Debug(int mode){
	if(mode==0 || mode==3){
		std::cout << "Timing calibration:\n";
		for(size_t i = 0; i < time_calib.size(); i++)
			std::cout << time_calib[i].Print() << std::endl;
	}
	if(mode==1 || mode==3){
		std::cout << "Energy calibration:\n";
		for(size_t i = 0; i < energy_calib.size(); i++)
			std::cout << energy_calib[i].Print() << std::endl;
	}
	if(mode==2 || mode==3){
		std::cout << "Position calibration:\n";
		for(size_t i = 0; i < position_calib.size(); i++)
			std::cout << position_calib[i].Print() << std::endl;
	}
}

bool CalibFile::Write(TFile *f_){
	if(!f_ || !f_->IsOpen())
		return false;

	// Make the calibration directory.
	f_->mkdir("calib");

	// Make the time calibration directory.
	f_->mkdir("calib/time");
	f_->cd("calib/time");

	for(std::vector<TimeCal>::iterator iter = time_calib.begin(); iter != time_calib.end(); ++iter){
		if(iter->defaultVals) continue; // Skip entries with default values.
		TObjString str(iter->Print(false).c_str());
		str.Write();
	}

	// Make the energy calibration directory.
	f_->mkdir("calib/energy");
	f_->cd("calib/energy");

	for(std::vector<EnergyCal>::iterator iter = energy_calib.begin(); iter != energy_calib.end(); ++iter){
		if(iter->defaultVals) continue; // Skip entries with default values.
		TObjString str(iter->Print(false).c_str());
		str.Write();
	}
	
	// Make the position calibration directory.
	f_->mkdir("calib/position");
	f_->cd("calib/position");

	for(std::vector<PositionCal>::iterator iter = position_calib.begin(); iter != position_calib.end(); ++iter){
		if(iter->defaultVals) continue; // Skip entries with default values.
		TObjString str(iter->Print(false).c_str());
		str.Write();
	}

	// Write bar calibration information to output file.
	f_->mkdir("calib/bars");
	f_->cd("calib/bars");

	// Write individual bar calibration entries.
	for(std::vector<BarCal>::iterator iter = bar_calib.begin(); iter != bar_calib.end(); ++iter){
		if(iter->defaultVals) continue; // Skip entries with default values.
		TObjString str(iter->Print(false).c_str());
		str.Write();
	}
	
	// Return to the top level directory.
	f_->cd();

	return true;
}

unsigned int GetPixieID(const std::string &str_){
	size_t commaIndex = str_.find(',');
	if(commaIndex == std::string::npos)
		return strtoul(str_.c_str(), NULL, 10);
	unsigned int pixieMod = strtol(str_.substr(0, commaIndex).c_str(), NULL, 10);
	unsigned int pixieChan = strtol(str_.substr(commaIndex+1).c_str(), NULL, 10);
	return (pixieMod*16 + pixieChan);
}
