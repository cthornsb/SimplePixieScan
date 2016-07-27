#####################################################################

# Set the binary install directory.
INSTALL_DIR = $(HOME)/bin

#####################################################################

CFLAGS = -g -Wall -std=c++0x
#CFLAGS = -Wall -O3 -std=c++0x
RFLAGS = `root-config --cflags --glibs`

COMPILER = g++

# Directories
TOP_LEVEL = $(shell pwd)

INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
EXEC_DIR = $(TOP_LEVEL)/exec

# Tools
TIME_ALIGN = $(EXEC_DIR)/timeAlign.a

ALL_TOOLS = $(TIME_ALIGN)

########################################################################

all: $(EXEC_DIR) $(ALL_TOOLS)
#	Create all directories, make all objects, and link executables

########################################################################

$(EXEC_DIR):
#	Make the executable directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi
	
########################################################################

$(EXEC_DIR)/%.a: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) $(CFLAGS) $< $(RFLAGS) -o $@

########################################################################

install: $(ALL_TOOLS)
	@echo " Installing to "$(INSTALL_DIR)
	@ln -s $(TIME_ALIGN) $(INSTALL_DIR)/timeAlign

########################################################################

uninstall: clean
	@rm -f $(INSTALL_DIR)/timeAlign

clean:
	@echo "Cleaning up..."
	@rm -f $(EXEC_DIR)/*.a
