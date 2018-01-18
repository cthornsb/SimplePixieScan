#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

/** Get the file extension from an input filename string.
  * \param[in]  filename_ The full input filename path.
  * \param[out] prefix	The input filename path without the file extension.
  * \return The file extension string.
  */
std::string get_extension(std::string filename_, std::string &prefix);

/** Check whether or not a cstring is a numeric value containing a decimal.
  * \return True if the input cstring is a numeric value with a decimal and false otherwise.
  */
bool isDecimal(const char *str_);

/** Calculate the parameters for a second order polynomial which passes through 3 points.
  * \param[in]  x0 - Initial x value. Sequential x values are assumed to be x0, x0+1, and x0+2.
  * \param[in]  y  - Pointer to the beginning of the array of unsigned shorts containing the three y values.
  * \param[out] p  - Pointer to the array of doubles for storing the three polynomial parameters.
  * \return The maximum/minimum of the polynomial.
  */
double calculateP2(const short &x0, unsigned short *y, double *p);

/** Calculate the parameters for a third order polynomial which passes through 4 points.
  * \param[in]  x0 - Initial x value. Sequential x values are assumed to be x0, x0+1, x0+2, and x0+3.
  * \param[in]  y  - Pointer to the beginning of the array of unsigned shorts containing the four y values.
  * \param[out] p  - Pointer to the array of doubles for storing the three polynomial parameters.
  * \return The local maximum/minimum of the polynomial.
  */
double calculateP3(const short &x0, unsigned short *y, double *p);

#endif
