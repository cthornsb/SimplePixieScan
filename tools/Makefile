#####################################################################

# Set the PixieSuite core directory.
PIXIE_SUITE_DIR = $(HOME)/opt/paass/install

# Set the include directory.
PIXIE_SUITE_INC_DIR = $(PIXIE_SUITE_DIR)/include

# Set the lib directory.
PIXIE_SUITE_LIB_DIR = $(PIXIE_SUITE_DIR)/lib

# Set the binary install directory.
INSTALL_DIR = $(HOME)/bin

#####################################################################

CFLAGS = -g -Wall -std=c++0x -Iinclude -I$(PIXIE_SUITE_INC_DIR)
#CFLAGS = -Wall -O3 -std=c++0x
RFLAGS = `root-config --cflags --glibs`
LDLIBS = `root-config --libs` -L$(PIXIE_SUITE_LIB_DIR) -lScan

COMPILER = g++

# Directories
TOP_LEVEL = $(shell pwd)

INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
EXEC_DIR = $(TOP_LEVEL)/exec
OBJ_DIR = $(TOP_LEVEL)/obj

# Objects to be compiled.
BUILD_OBJ = $(OBJ_DIR)/simpleTool.o

# Tools
TIME_ALIGN = $(EXEC_DIR)/timeAlign.a
CALIBRATE = $(EXEC_DIR)/calibrate.a

ALL_TOOLS = $(TIME_ALIGN) $(CALIBRATE)

########################################################################

all: $(EXEC_DIR) $(OBJ_DIR) $(BUILD_OBJ) $(ALL_TOOLS)
#	Create all directories, make all objects, and link executables

########################################################################

$(OBJ_DIR):
#	Make the object file directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi

$(EXEC_DIR):
#	Make the executable directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi
	
########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) $(CFLAGS) -c $< $(RFLAGS) -o $@

$(EXEC_DIR)/%.a: $(SOURCE_DIR)/%.cpp $(BUILD_OBJ)
#	Compile C++ source files
	$(COMPILER) $(CFLAGS) $< $(RFLAGS) $(BUILD_OBJ) -o $@ $(LDLIBS)

########################################################################

install: $(ALL_TOOLS)
	@echo " Installing to "$(INSTALL_DIR)
	@ln -s $(TIME_ALIGN) $(INSTALL_DIR)/timeAlign
	@ln -s $(CALIBRATE) $(INSTALL_DIR)/calibrate

########################################################################

uninstall: clean
	@rm -f $(INSTALL_DIR)/timeAlign
	@rm -f $(INSTALL_DIR)/calibrate

clean:
	@echo "Cleaning up..."
	@rm -f $(EXEC_DIR)/*.a $(OBJ_DIR)/*.o
