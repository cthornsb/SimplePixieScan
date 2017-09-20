#ifndef COLOR_TERM_HPP
#define COLOR_TERM_HPP

#include <iostream>

class ErrorStream {
  public:
	ErrorStream() { }
  
	template <typename T>
	const ErrorStream& operator<<(const T &v) const { 
		std::cout << "\033[1;31m" << v << "\033[0m";
		return *this;
	}
};

class WarningStream {
  public:
	WarningStream() { }
  
	template <typename T>
	const WarningStream& operator<<(const T &v) const { 
		std::cout << "\033[1;33m" << v << "\033[0m";
		return *this;
	}
};

class NoteStream {
  public:
	NoteStream() { }
  
	template <typename T>
	const NoteStream& operator<<(const T &v) const { 
		std::cout << "\033[1;34m" << v << "\033[0m";
		return *this;
	}
};

extern ErrorStream errStr;

extern WarningStream warnStr;

extern NoteStream noteStr;

#endif
