#ifndef PARENTCLASS_HPP
#define PARENTCLASS_HPP

#include <iostream>
#include <vector>
#include <string>

class ParentClass{
  protected:
	std::string class_name;
	bool debug_mode;
	bool init;
	
  public:
	ParentClass(std::string name_);

	bool IsInit(){ return init; }
	
	void SetDebugMode(bool debug_=true){ debug_mode = debug_; }
	
	void PrintMsg(const std::string &msg_);
	
	void PrintError(const std::string &msg_);
	
	void PrintWarning(const std::string &msg_);
	
	void PrintNote(const std::string &msg_);
};

#endif
