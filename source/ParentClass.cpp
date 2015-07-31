#include <sstream>

#include "ParentClass.hpp"

//extern const bool use_color_terminal;
const bool use_color_terminal = true;

ParentClass::ParentClass(std::string name_){ 
	class_name = name_; 
	debug_mode = false;
	init = false;
}

void ParentClass::PrintMsg(const std::string &msg_){
	std::cout << class_name << ": " << msg_ << std::endl; 
}

void ParentClass::PrintError(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;31m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << class_name << ": " << msg_ << std::endl; }
}

void ParentClass::PrintWarning(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;33m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << class_name << ": " << msg_ << std::endl; }
}

void ParentClass::PrintNote(const std::string &msg_){ 
	if(use_color_terminal){ std::cout << "\e[1;34m" << class_name << ": " << msg_ << "\e[0m" << std::endl; }
	else{ std::cout << class_name << ": " << msg_ << std::endl; }
}
