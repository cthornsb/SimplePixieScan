#include <string>
#include <string.h>
#include <cmath>

#include "helperFunctions.h"

/** Get the file extension from an input filename string.
  * \param[in]  filename_ The full input filename path.
  * \param[out] prefix	The input filename path without the file extension.
  * \return The file extension string.
  */
std::string get_extension(std::string filename_, std::string &prefix){
	size_t count = 0;
	size_t last_index = 0;
	std::string output = "";
	prefix = "";
	
	if(filename_.find('.') != std::string::npos){
		// Find the final period in the filename
		for(count = 0; count < filename_.size(); count++){
			if(filename_[count] == '.'){ last_index = count; }
		}
	
		// Get the filename prefix and the extension
		for(size_t i = 0; i < count; i++){
			if(i < last_index){ prefix += filename_[i]; }
			else if(i > last_index){ output += filename_[i]; }
		}
	}
	else{ // The filename has no extension.
		prefix = filename_;
	}
	
	return output;
}

/** Check whether or not a cstring is a numeric value containing a decimal.
  * \return True if the input cstring is a numeric value with a decimal and false otherwise.
  */
bool isDecimal(const char *str_){
	size_t length = strlen(str_);
	bool retval = false;
	for(size_t i = 0; i < length; i++){
		if(str_[i] == '.'){
			retval = true;
			continue;
		}
		else if(str_[i] < 0x30 || str_[i] > 0x39) return false; // Not a number.
	}
	return retval;
}
/** Calculate the parameters for a second order polynomial which passes through 3 points.
  * \param[in]  x0 - Initial x value. Sequential x values are assumed to be x0, x0+1, and x0+2.
  * \param[in]  y  - Pointer to the beginning of the array of unsigned shorts containing the three y values.
  * \param[out] p  - Pointer to the array of doubles for storing the three polynomial parameters.
  * \return The maximum/minimum of the polynomial.
  */
double calculateP2(const short &x0, unsigned short *y, double *p){
	double x1[3], x2[3];
	for(size_t i = 0; i < 3; i++){
		x1[i] = (x0+i);
		x2[i] = std::pow(x0+i, 2);
	}

	double denom = (x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(x2[2]-x2[1]*1) + x2[0]*(x1[2]-x1[1]*1);

	p[0] = (y[0]*(x1[1]*x2[2]-x2[1]*x1[2]) - x1[0]*(y[1]*x2[2]-x2[1]*y[2]) + x2[0]*(y[1]*x1[2]-x1[1]*y[2]))/denom;
	p[1] = ((y[1]*x2[2]-x2[1]*y[2]) - y[0]*(x2[2]-x2[1]*1) + x2[0]*(y[2]-y[1]*1))/denom;
	p[2] = ((x1[1]*y[2]-y[1]*x1[2]) - x1[0]*(y[2]-y[1]*1) + y[0]*(x1[2]-x1[1]*1))/denom;
	
	// Calculate the maximum of the polynomial.
	return (p[0] - p[1]*p[1]/(4*p[2]));
}

/** Calculate the parameters for a third order polynomial which passes through 4 points.
  * \param[in]  x0 - Initial x value. Sequential x values are assumed to be x0, x0+1, x0+2, and x0+3.
  * \param[in]  y  - Pointer to the beginning of the array of unsigned shorts containing the four y values.
  * \param[out] p  - Pointer to the array of doubles for storing the three polynomial parameters.
  * \return The local maximum/minimum of the polynomial.
  */
double calculateP3(const short &x, unsigned short *y, double *p){
	double x1[4], x2[4], x3[4];
	for(size_t i = 0; i < 4; i++){
		x1[i] = (x+i);
		x2[i] = std::pow(x+i, 2);
		x3[i] = std::pow(x+i, 3);
	}

	double denom = (x1[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + x1[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - (x1[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + (x1[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - x1[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - (x1[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - x1[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + x1[2]*(x2[0]*x3[1]-x2[1]*x3[0]));

	p[0] = (y[0]*(x1[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + x1[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - y[1]*(x1[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - x1[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + y[2]*(x1[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - x1[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + x1[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - y[3]*(x1[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - x1[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + x1[2]*(x2[0]*x3[1]-x2[1]*x3[0]))) / denom;
	p[1] = ((y[1]*(x2[2]*x3[3]-x2[3]*x3[2]) - y[2]*(x2[1]*x3[3]-x2[3]*x3[1]) + y[3]*(x2[1]*x3[2]-x2[2]*x3[1])) - (y[0]*(x2[2]*x3[3]-x2[3]*x3[2]) - y[2]*(x2[0]*x3[3]-x2[3]*x3[0]) + y[3]*(x2[0]*x3[2]-x2[2]*x3[0])) + (y[0]*(x2[1]*x3[3]-x2[3]*x3[1]) - y[1]*(x2[0]*x3[3]-x2[3]*x3[0]) + y[3]*(x2[0]*x3[1]-x2[1]*x3[0])) - (y[0]*(x2[1]*x3[2]-x2[2]*x3[1]) - y[1]*(x2[0]*x3[2]-x2[2]*x3[0]) + y[2]*(x2[0]*x3[1]-x2[1]*x3[0]))) / denom;
	p[2] = ((x1[1]*(y[2]*x3[3]-y[3]*x3[2]) - x1[2]*(y[1]*x3[3]-y[3]*x3[1]) + x1[3]*(y[1]*x3[2]-y[2]*x3[1])) - (x1[0]*(y[2]*x3[3]-y[3]*x3[2]) - x1[2]*(y[0]*x3[3]-y[3]*x3[0]) + x1[3]*(y[0]*x3[2]-y[2]*x3[0])) + (x1[0]*(y[1]*x3[3]-y[3]*x3[1]) - x1[1]*(y[0]*x3[3]-y[3]*x3[0]) + x1[3]*(y[0]*x3[1]-y[1]*x3[0])) - (x1[0]*(y[1]*x3[2]-y[2]*x3[1]) - x1[1]*(y[0]*x3[2]-y[2]*x3[0]) + x1[2]*(y[0]*x3[1]-y[1]*x3[0]))) / denom;
	p[3] = ((x1[1]*(x2[2]*y[3]-x2[3]*y[2]) - x1[2]*(x2[1]*y[3]-x2[3]*y[1]) + x1[3]*(x2[1]*y[2]-x2[2]*y[1])) - (x1[0]*(x2[2]*y[3]-x2[3]*y[2]) - x1[2]*(x2[0]*y[3]-x2[3]*y[0]) + x1[3]*(x2[0]*y[2]-x2[2]*y[0])) + (x1[0]*(x2[1]*y[3]-x2[3]*y[1]) - x1[1]*(x2[0]*y[3]-x2[3]*y[0]) + x1[3]*(x2[0]*y[1]-x2[1]*y[0])) - (x1[0]*(x2[1]*y[2]-x2[2]*y[1]) - x1[1]*(x2[0]*y[2]-x2[2]*y[0]) + x1[2]*(x2[0]*y[1]-x2[1]*y[0]))) / denom;

	if(p[3] == 0){
		// Handle the case of p[3] == 0.
		return (p[0] - p[1]*p[1]/(4*p[2]));
	}

	// Calculate the maximum of the polynomial.
	double xmax1 = (-2*p[2]+std::sqrt(4*p[2]*p[2]-12*p[3]*p[1]))/(6*p[3]);
	double xmax2 = (-2*p[2]-std::sqrt(4*p[2]*p[2]-12*p[3]*p[1]))/(6*p[3]);

	if((2*p[2]+6*p[3]*xmax1) < 0) // The second derivative is negative (i.e. this is a maximum).
		return (p[0] + p[1]*xmax1 + p[2]*xmax1*xmax1 + p[3]*xmax1*xmax1*xmax1);

	return (p[0] + p[1]*xmax2 + p[2]*xmax2*xmax2 + p[3]*xmax2*xmax2*xmax2);
}
