#ifndef BARCAL_HPP
#define BARCAL_HPP

#include <string>
#include <vector>

class barCal : public CalType {
  public:
	double t0;
	double beta;
	double cbar;
	double length;
	double width;

	barCal() : CalType(0), t0(0), beta(0), cbar(0), length(0), width(0) { }

	virtual std::string Print(bool fancy=true);

	virtual void ReadPars(const std::vector<std::string> &pars_);
};

#endif
