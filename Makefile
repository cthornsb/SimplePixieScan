#####################################################################

# Set the PixieSuite core directory.
PIXIE_SUITE_DIR = $(HOME)/opt/paass/install

# Set the include directory.
PIXIE_SUITE_INC_DIR = $(PIXIE_SUITE_DIR)/include

# Set the lib directory.
PIXIE_SUITE_LIB_DIR = $(PIXIE_SUITE_DIR)/lib

# Set the binary install directory.
INSTALL_DIR = $(HOME)/bin

# Set the library install directory.
LIB_INSTALL_DIR = $(HOME)/lib

#####################################################################

# Set the directory where you plan to run the scan.
SCAN_DIR = $(HOME)/scan

# Set the directory containing runtime configuration files.
CONFIG_DIR = $(SCAN_DIR)/config/default

#####################################################################

# Uncomment to use scanor from HHIRF UPAK instead of ScanInterface.
#USE_SCANOR_READOUT = 1

# Set the HHIRF (UPAK) install directory. 
# Only used for scanor readout.
HHIRF_DIR = $(HOME)/opt/hhirf

#####################################################################
#              NO USER OPTIONS BELOW THIS POINT                     #
#####################################################################

CFLAGS = -g
#CFLAGS = -O3

LDLIBS = -lstdc++ -L$(PIXIE_SUITE_LIB_DIR) -lScan
LDFLAGS = `root-config --glibs`

# Add the USE_HRIBF preprocessor flag to the CFLAGS variable.
ifeq ($(USE_SCANOR_READOUT), 1)
	CFLAGS += -DUSE_HRIBF
	LDLIBS += -lScanHHIRF
endif

CFLAGS += -Wall -std=c++0x `root-config --cflags`
LDLIBS += `root-config --libs`

COMPILER = g++
LINKER = g++
ifeq ($(USE_SCANOR_READOUT), 1)
	LINKER = gfortran
endif

# Directories
TOP_LEVEL = $(shell pwd)

INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
OBJ_DIR = $(TOP_LEVEL)/obj

DICT_DIR = $(TOP_LEVEL)/dict
DICT_OBJ_DIR = $(DICT_DIR)/obj

EXEC_DIR = $(TOP_LEVEL)/exec

RCBUILD_DIR = $(TOP_LEVEL)/rcbuild

# List of directories to generate if they do not exist.
DIRECTORIES = $(OBJ_DIR) $(EXEC_DIR) $(CONFIG_DIR)

# Core files
SOURCES = Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp \
          ConfigFile.cpp MapFile.cpp CalibFile.cpp Scanner.cpp

# Processors
SOURCES += TriggerProcessor.cpp \
           VandleProcessor.cpp \
           PhoswichProcessor.cpp \
           LiquidBarProcessor.cpp \
           LiquidProcessor.cpp \
           HagridProcessor.cpp \
           GenericProcessor.cpp \
           LogicProcessor.cpp \
           TraceProcessor.cpp

OBJECTS = $(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.o))

# Configuration files
CONFIG_FILES = config.dat energy.cal map.dat position.cal time.cal

# ROOT dictionary stuff
DICT_SOURCE = SimpleScanDict
PCM_FILE = $(DICT_SOURCE)_rdict.pcm
STRUCT_FILE_OBJ = $(OBJ_DIR)/Structures.o
DICT_SHARED_LIB = lib$(DICT_SOURCE).so

ROOT_DICT = $(DICT_DIR)/$(DICT_SOURCE).cpp
ROOT_DICT_OBJ = $(DICT_OBJ_DIR)/$(DICT_SOURCE).o
ROOT_DICT_SLIB = $(DICT_OBJ_DIR)/lib$(DICT_SOURCE).so
SFLAGS = $(addprefix -l,$(DICT_SOURCE))

OBJECTS += $(ROOT_DICT_OBJ) $(STRUCT_FILE_OBJ)

SCANOR_OBJS = $(HHIRF_DIR)/scanorlib.a \
              $(HHIRF_DIR)/orphlib.a \
              $(HHIRF_DIR)/acqlib.a \
              $(HHIRF_DIR)/ipclib.a
               
EXECUTABLE = $(EXEC_DIR)/PixieLDF
ifeq ($(USE_SCANOR_READOUT), 1)
	EXECUTABLE = $(EXEC_DIR)/PixieLDF_s
endif

########################################################################

all: config dictionary runscript $(EXECUTABLE)
#	Create all directories, make all objects, and link executable

dictionary:
#	Create root dictionary objects
	@$(RCBUILD_DIR)/rcbuild.sh
#	Copy the root .pcm file (if it exists) to the top directory
	@if [ -e $(DICT_DIR)/$(DICT_SOURCE)_rdict.pcm ]; then \
		cp $(DICT_DIR)/$(DICT_SOURCE)_rdict.pcm $(TOP_LEVEL)/; \
	fi
	
runscript:
#	Generate the run.sh shell script
	@$(RCBUILD_DIR)/libpath.sh $(PIXIE_SUITE_LIB_DIR) $(DICT_OBJ_DIR) $(EXECUTABLE)
 
.PHONY: $(CONFIG_FILES) $(DIRECTORIES)
 
########################################################################

$(CONFIG_FILES): 
#	Unpack the default configuration files, if needed.
	@if [ ! -e $(CONFIG_DIR)/$@ ]; then \
		tar -xf $(TOP_LEVEL)/config.tar -C $(CONFIG_DIR) $@; \
		echo "Unpacking "$@" to "$(CONFIG_DIR); \
	fi

$(DIRECTORIES): 
#	Make the default configuration directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir -p $@; \
	fi
	
config: $(DIRECTORIES) $(CONFIG_FILES)
#	Create a symbolic link to the default config directory
	@if [ ! -e $(SCAN_DIR)/setup ]; then \
		ln -s $(CONFIG_DIR) $(SCAN_DIR)/setup; \
		echo "Creating symbolic link to "$(CONFIG_DIR)" in "$(SCAN_DIR); \
	fi

link: $(DIRECTORIES) $(CONFIG_FILES)
#	Force the creation a symbolic link to the default config directory
	@rm -f $(SCAN_DIR)/setup
	@if [ ! -e $(SCAN_DIR)/setup ]; then \
		ln -s $(CONFIG_DIR) $(SCAN_DIR)/setup; \
		echo "Creating symbolic link to "$(CONFIG_DIR)" in "$(SCAN_DIR); \
	fi

########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) -c $(CFLAGS) -Iinclude -I$(PIXIE_SUITE_INC_DIR) $< -o $@

########################################################################

$(EXECUTABLE): $(OBJECTS)
ifneq ($(USE_SCANOR_READOUT), 1)
	$(LINKER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS)
else
	$(LINKER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS) $(SCANOR_OBJS)
endif

########################################################################

install: all
	@rm -f $(INSTALL_DIR)/simpleScan $(LIB_INSTALL_DIR)/$(DICT_SHARED_LIB)
	@echo "Installing simpleScan to "$(INSTALL_DIR)
	@ln -s $(TOP_LEVEL)/run.sh $(INSTALL_DIR)/simpleScan;
	@echo "Installing libSimpleScanDict.so to "$(LIB_INSTALL_DIR)
	@ln -s $(ROOT_DICT_SLIB) $(LIB_INSTALL_DIR)/$(DICT_SHARED_LIB)
	@if [ -d $(SCAN_DIR) ]; then \
		if [ -e $(PCM_FILE) ]; then \
			cp $(PCM_FILE) $(SCAN_DIR); \
			echo "Copying root PCM file "$(PCM_FILE)" to "$(SCAN_DIR); \
		fi \
	fi

########################################################################

uninstall: tidy
	@rm -f $(INSTALL_DIR)/simpleScan
	@rm -f $(LIB_INSTALL_DIR)/$(DICT_SHARED_LIB)

tidy: clean_obj clean_dict
	@rm -f $(EXEC_DIR)/* ./run.sh

clean: clean_obj

clean_obj:
	@echo "Cleaning up..."
	@rm -f $(OBJ_DIR)/*.o
	
clean_dict:
	@echo "Removing ROOT dictionaries..."
	@rm -f $(DICT_DIR)/$(DICT_SOURCE).cpp $(DICT_DIR)/$(DICT_SOURCE).h $(DICT_OBJ_DIR)/*.o  $(DICT_OBJ_DIR)/*.so
	@rm -f $(STRUCT_FILE_OBJ) $(SOURCE_DIR)/Structures.cpp $(INCLUDE_DIR)/Structures.h $(DICT_DIR)/LinkDef.h
