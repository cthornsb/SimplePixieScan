#ifndef OPTIONHANDLER_HPP
#define OPTIONHANDLER_HPP

#include <string>
#include <vector>
#include <getopt.h>

class optionExt{
  public:
	const char* name;
	int has_arg;
	int *flag;
	int val;
  
  	std::string argstr; /// The argument syntax for the command line option.
	std::string helpstr; /// The help & syntax string to print when -h is passed.
	std::string argument; /// The argument received from getopt_long (if available).
	bool active; /// Set to true if this option was selected by the user.
	
	optionExt() : has_arg(0), flag(0), val(0), active(false) { }
	
	optionExt(const char *name_, const int &has_arg_, int *flag_, const int &val_, const std::string &arg_, const std::string &help_);
	
	/// Print a help string for this option.
	void print(const size_t &len_=0, const std::string &prefix_="");
	
	option getOption();
};

class optionHandler{
  public:
	optionHandler();
	
	~optionHandler(){ }

	/** SyntaxStr is used to print a linux style usage message to the screen.
	  * Prints a standard usage message by default.
	  * \param[in]  name_ The name of the program.
	  * \return Nothing.
	  */
	void syntaxStr(char *name_);

	/** Print a command line argument help dialogue.
	  * \param[in]  name_ The name of the program.
	  * \return Nothing.
	  */  
	void help(char *name_);
	
	/** Add a command line option to the option list.
	  * \param[in]  opt_ The option to add to the list.
	  * \return Nothing.
	  */
	void add(optionExt opt_);

	/** Setup user options and initialize all required objects.
	  * \param[in]  argc Number of arguments passed from the command line.
	  * \param[in]  argv Array of strings passed as arguments from the command line.
	  * \return True upon success and false otherwise.
	  */
	bool setup(int argc, char *argv[]);

	optionExt *getOption(const size_t &index_);

  private:
	std::vector<option> longOpts; /// Vector of all command line options.
	std::vector<optionExt> baseOpts; /// Default command line options.
	std::vector<optionExt> userOpts; /// User added command line options.
	std::string optstr;
};

/** Add a command line option to the option list.
  * \param[in]  opt_ The option to add to the list.
  * \return Nothing.
  */
void addOption(optionExt opt_, std::vector<optionExt> &vec, std::string &optstr);

#endif
