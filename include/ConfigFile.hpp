#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

class TFile;

class ConfigFile{
  private:
	bool init;
  
	void parse_string(const std::string &input_, std::string &name, std::string &value);

  public:
	double adcClock;
	double sysClock;
	double eventWidth;
	double eventDelay;
	int buildMethod;
  
	ConfigFile();
	
	ConfigFile(const char *filename_);
	
	bool IsInit(){ return init; }
	
	bool Load(const char *filename_);
	
	bool Write(TFile *f_);
};

#endif
