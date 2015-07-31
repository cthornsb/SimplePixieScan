#ifndef CONFIGFILE_HPP
#define CONFIGFILE_HPP

#include "ParentClass.hpp"

class ConfigFile : public ParentClass{
  private:
	void parse_string(const std::string &input_, std::string &name, std::string &value);

  public:
	float event_width;
  
	ConfigFile();
	
	ConfigFile(const char *filename_);
	
	bool Load(const char *filename_);
};

#endif
