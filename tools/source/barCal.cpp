#include <sstream>

#include "CalibFile.hpp"
#include "barCal.hpp"

std::string barCal::Print(bool fancy/*=true*/){
	std::stringstream output;
	if(fancy) output << " id=" << id << ", t0=" << t0 << ", beta=" << beta << ", cbar=" << cbar << ", length=" << length << ", width=" << width;
	else output << id << "\t" << t0 << "\t" << beta << "\t" << cbar << "\t" << length << "\t" << width;
	return output.str();
}

void barCal::ReadPars(const std::vector<std::string> &pars_){
	defaultVals = false;
	int index = 0;
	for(std::vector<std::string>::const_iterator iter = pars_.begin(); iter != pars_.end(); iter++){
		if(index == 0) id = strtol(iter->c_str(), NULL, 0);
		else if(index == 1) t0 = strtod(iter->c_str(), NULL);
		else if(index == 2) beta = strtod(iter->c_str(), NULL);
		else if(index == 3) cbar = strtod(iter->c_str(), NULL);
		else if(index == 4) length = strtod(iter->c_str(), NULL);
		else if(index == 5) width = strtod(iter->c_str(), NULL);
		index++;
	}
}
