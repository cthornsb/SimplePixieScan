/** @file CTerminal.h
  * 
  * @brief Library to handle all aspects of a stand-alone command line interface
  *
  * Library to facilitate the creation of C++ executables with
  * interactive command line interfaces under a linux environment
  *
  * @author Cory R. Thornsberry
  * @author Karl Smith
  * 
  * @date Oct. 1st, 2015
  * 
  * @version 1.2.03
*/

#ifndef CTERMINAL_H
#define CTERMINAL_H

#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <vector>
#include <deque>

#define SCROLLBACK_SIZE 1000 ///< Default size of terminal scroll back buffer in lines

#define CTERMINAL_VERSION "1.2.10"
#define CTERMINAL_DATE "Sept. 7th, 2016"

#include <curses.h>

extern bool SIGNAL_SEGFAULT; ///< Flag indicating that the system has passed segmentation fault signal (SIGSEGV)
extern bool SIGNAL_INTERRUPT; ///< Flag indicating that the system has passed program interrupt signal (ctrl-c, SIGINT)
extern bool SIGNAL_TERMSTOP; ///< Flag indicating that the system has passed program stop signal (ctrl-z, SIGTSTP)
extern bool SIGNAL_RESIZE; ///< Flag indicating that the user has modified the size of the window

//extern std::string CPP_APP_VERSION;

/** Convert arbitrary input to a string
  */
template <typename T>
std::string to_str(const T &input_);

class CommandHolder{
  private:
	unsigned int max_size; ///< Maximum number of commands to keep in history
	unsigned int index; ///< The index of the current command in the command history
	unsigned int total; ///< Total number of commands entered into the history
	unsigned int external_index; ///< The current history index with respect to the current command index
	std::string fragment; ///< Partial input which the user enters without pressing enter
	
	std::vector<std::string> commands; ///< List of past commands the user has entered

	/** Convert the external index (relative to the most recent command) to the internal index which is used to actually access the stored commands in the command array
	  * @return 
	  */
	unsigned int wrap_();

  public:
	/** Default constructor
	  * @param max_size_ Maximum number of lines to keep in the command history
	  */
	CommandHolder(unsigned int max_size_=1000) : max_size(max_size_), index(0), total(0), external_index(0), fragment(""), commands(max_size_, "") { }
	
	/** Destructor
	  */
	~CommandHolder(){ }
	
	/** Get the maximum size of the command array
	  */
	unsigned int GetSize(){ return max_size; }
	
	/** Get the total number of commands which have been entered
	  */
	unsigned int GetTotal(){ return total; }
	
	/** Get the current command index (relative to the most recent command)
	  */
	unsigned int GetIndex(){ return external_index; }
	
	/** Push a new command into the storage array
	  * @param input_ User input line from the terminal
	  */
	void Push(std::string &input_);
	
	/** Capture the current command line text fragment and store it for later use
	  */
	void Capture(const std::string &input_){ fragment = input_; }
		
	/** Clear all entered commands
	  */
	void Clear();
	
	/** Get the previous command entry
	  */
	std::string GetPrev();

	/** Get the previous command entry but do not change to the index of that command
	  */
	std::string PeekPrev();
	
	/** Get the next command entry
	  */
	std::string GetNext();

	/** Get the next command entry but do not change to the index of that command
	  */
	std::string PeekNext();
	
	/** Dump all entered commands to the screen
	  */
	void Dump();

	/** Reset history to most recently entered command
	  */
	void Reset();
};

/** Function which will be called when the system passes signal SIGSEGV
  * @param ignore_ Not used
  */
void sig_segv_handler(int ignore_);

/** Function which will be called when the system passes signal SIGINT
  * @param ignore_ Not used
  */
void sig_int_handler(int ignore_);

/** Function which will be called when the system passes signal SIGTSTP
  * @param ignore_ Not used
  */
void sig_tstp_handler(int ignore_);

/** Function which will be called when the user modifies the size of the window
  * @param ignore_ Not used
  */
void signalResize(int ignore_);

/** Setup all system signal intercepts
  */
void setup_signal_handlers();

/** Unset all system signal intercepts
  */
void unset_signal_handlers();

/** Check wheter the system issued SIGSEGV, SIGINT, SIGTSTP, or SIGWINCH signals
  * @return The system code if a system signal was thrown and return -1 otherwise
  */
int check_signals();

class Terminal{
  private:
	std::streambuf *pbuf; ///< Pointer to stringstream buffer
	std::streambuf *original; ///< Pointer to the original stream buffer of std::cout		
		
	WINDOW *main; ///< Main window
	WINDOW *output_window; ///< Window used for text output
	WINDOW *input_window; ///< Window used for text input from the user
	WINDOW *status_window; ///< Window used for status messages

	int cursX; ///< Horizontal position of the physical cursor
	int cursY; ///< Vertical position of the physical cursor
	int offset; ///< Character offset due to the command prompt
	int _winSizeX; ///< The current horizontal size of the window
	int _winSizeY; ///< The current vertical size of the window
	int _statusWindowSize; ///< The vertical size of the status window (in lines)
	
	bool init; ///< Flag indicating that the terminal has been initialized
	bool enableTabComplete; ///< Flag indicating that tab complete is enabled
	bool doResizeWindow; ///< Flag indicating that the user has modified the size of the window
	bool insertMode_; ///< Flag indicating that the terminal is in character insert mode
	bool debug_; ///< Flag indicating verbose output is enabled
	bool from_script; ///< Flag indciating that a command is read from a script instead of user input
	bool prompt_user; ///< Flag indicating that the user should be prompted with a yes/no question	
		
	short tabCount; ///< The number of times the user has pressed the tab key sequentially	
	
	float commandTimeout_; ///< Time to wait for command before timeout (in seconds)		
		
	int _scrollbackBufferSize; ///< Size of the scroll back buffer in lines
	int _scrollPosition; ///< Number of lines scrolled back	
	
	std::map< std::string, int > attrMap; ///< Map of terminal color escape sequences
	
	std::stringstream stream; ///< Stream used to redirect std::cout
	std::string historyFilename_; ///< Path to the command history file
	std::string cmd; ///< User command input string	
	
	CommandHolder commands; ///< Command history handler

	std::vector<std::string> statusStr; ///< Vector containing the status window strings
	std::deque<std::string> cmd_queue; ///< The queue of commands read from a command script

	std::string prompt; ///< The command prompt string

	std::ofstream logFile; ///< Output file used for logging terminal output
	
	/** Refresh all defined windows
	  */
	void refresh_();

	/** Resize the terminal
	  */
	void resize_();

	/** Scroll the output by a specified number of lines
	  */
	void scroll_(int numLines);
	
	/** Update the positions of the physical and logical cursors
	  */
	void update_cursor_();
	
	/** Clear the command prompt output
	  */
	void clear_();
	
	/** Force a character to the input screen
	  */
	void in_char_(const char input_);

	/** Force a character string to the input screen
	  */
	void in_print_(const char *input_);

	/** Initialize terminal colors
	  */
	void init_colors_();
	
	/** Read commands from a command script
	  * An example CTerminal script is given below. Comments are denoted by a #.
	  * #############################
	  * # Tell the user something about this script.
	  * .echo This script is intended to be used to test the script reader (i.e. '.cmd').
	  * 
	  * # Issue the user a prompt asking if they would like to continue.
	  * # If the user types anything other than 'yes' or 'y', the script will abort.
	  * .prompt WARNING! This script will do something. Are you sure you wish to proceed?
	  * 
	  * help # Do something just to test that the script is working.
	  * #############################
	  *
	  * @param filename_ Path to the input command file
	  * @return True if the file is opened successfully and return false otherwise
	  */
	bool LoadCommandFile(const char *filename_);
	
	/** Load a list of previous commands from a file
	  * @param overwrite Flag indicating that commands currently in history will be deleted
	  * @return True if the history file is opened successfully and return false otherwise
	  */
	bool LoadCommandHistory(bool overwrite);
	
	/** Save command history to file
	  */
	bool SaveCommandHistory();

	/** Force a character string to the output screen
	  * @param window Pointer to the window region where the string should be printed
	  * @param input_ The string to print
	  */
	void print(WINDOW *window, std::string input_);

	/** Split a string into multiple commands separated by a ';'
	  * @param input_ User input string
	  * @param cmds Deque where the individual commands will be placed
	  */
	void split_commands(const std::string &input_, std::deque<std::string> &cmds);
			
  public:
	/** Default constructor
	  */
	Terminal();

	/** Destructor
	  */	
	~Terminal();
		
	/** Initialize the terminal interface
	  */
	void Initialize();
	
	/** Specify the filename of the output log file to append to
	  * @param logFileName Path to the output file
	  * @return True if the file is opened successfully and return false otherwise
	  */
	bool SetLogFile(std::string logFileName);
		
	/** Enable or disable terminal debug mode
	  */
	void SetDebug(bool debug=true) {debug_=debug;};

	/** Initalizes a status window under the input temrinal
	  * @param The number of vertical lines the status window will occupy
	  */
	void AddStatusWindow(unsigned short numLines = 1);
	
	/** Set the status message
	  * @param status The string to print to the status window
	  * @param line The index of the status window line to print text to
	  */
	void SetStatus(std::string status, unsigned short line = 0);
	
	/** Clear a line of the status window
	  * @param line The index of the status window line to clear
	  */
	void ClearStatus(unsigned short line = 0);
	
	/** Append text to the status message
	  * @param status The string to append to the status window
	  * @param line The index of the status window line to append text to
	  */
	void AppendStatus(std::string status, unsigned short line = 0);
		
	/** Enable or disable tab auto complete functionlity
	  * By enabling tab auto complete the current typed command is returned via GetCommand() with a trailing tab character.
	  */
	void EnableTabComplete(bool enable = true);

	/** Handle tab complete functionality
	  * If the list is empty nothing happens, if a unique value is given the command is completed. If there are multiple
	  * matches the common part of the matches is determined and printed to the input. If there is no common part of the
	  * matches and the tab key has been pressed twice the list of matches is printed for the user to decide.
	  *
	  * @param input_ Input string to be used to search for matches
	  * @param possibilities_ Vector of all possible matches
	  */
	void TabComplete(const std::string &input_, const std::vector<std::string> &possibilities_);

	/** Enable a timeout while waiting for a command
	  * By enabling the timeout the GetCommand() routine returns after a set
	  * timesout period has passed. The current typed text is stored for the next
	  * GetCommand() call
	  * 
	  * @param timeout The amount of time to wait before GetCommand() will timeout (in seconds)
	  */
	void EnableTimeout(float timeout = 0.5);

	/** Set the command filename for storing previous commands
	  * This command will clear all current commands from the history if overwrite is set to true
	  *
	  * @param filename The filename for the command history
	  * @param overwrite Flag indicating the current command history should be forgotten
	  */
	void SetCommandHistory(std::string filename, bool overwrite=false);
		
	/** Set the command prompt
	  */
	void SetPrompt(const char *input_);
	
	/** Force a character to the output screen
	  */
	void putch(const char input_);

	/** Disrupt ncurses while boolean is true
	  */
	void pause(bool &flag);

	/** Dump all text in the stream to the output screen
	  */
	void flush();

	/** Print a command to the terminal output
	  */
	void PrintCommand(const std::string &cmd_);

	/** Wait for the user to input a command
	  */
	std::string GetCommand(std::string &args, const int &prev_cmd_return_=0);
	
	/** Close the window and restore control to the terminal
	  */
	void Close();
};

/** Split a string about some delimiter
  * @param str Input string
  * @param args Vector to fill with substrings from the input string
  * @param delimiter Character to use for splitting the input string
  * @return The number of substrings found
  */
unsigned int split_str(std::string str, std::vector<std::string> &args, char delimiter=' ');

/** Get the length of a character string
  * @param str_ Pointer to the input character string
  * @return The length of the character string (not counting the terminating character)
  */
unsigned int cstrlen(const char *str_);

#endif
