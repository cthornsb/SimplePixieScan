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

# Use scanor from HHIRF UPAK instead of ScanInterface.
#USE_SCANOR_READOUT = 1

# Set the HHIRF (UPAK) install directory. 
# Only used for scanor readout.
HHIRF_DIR = $(HOME)/opt/hhirf

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

# Core files
SOURCES = Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp ConfigFile.cpp MapFile.cpp CalibFile.cpp Scanner.cpp

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

# ROOT dictionary stuff
DICT_SOURCE = RootDict
STRUCT_FILE_OBJ = $(OBJ_DIR)/Structures.o

ROOT_DICT = $(DICT_DIR)/$(DICT_SOURCE).cpp
ROOT_DICT_OBJ = $(DICT_OBJ_DIR)/$(DICT_SOURCE).o
ROOT_DICT_SLIB = $(DICT_OBJ_DIR)/$(DICT_SOURCE).so
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

all: directory dictionary runscript $(EXECUTABLE)
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

########################################################################

directory: $(OBJ_DIR) $(EXEC_DIR)
# Setup the configuration directory
	@if [ ! -d $(TOP_LEVEL)/config/default ]; then \
		tar -xf $(TOP_LEVEL)/config.tar; \
		echo "Building configuration directory"; \
	fi
# Create a symbolic link to the default config directory
	@if [ ! -e $(TOP_LEVEL)/setup ]; then \
		ln -s $(TOP_LEVEL)/config/default $(TOP_LEVEL)/setup; \
		echo "Creating symbolic link to default configuration directory"; \
	fi

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
	$(COMPILER) -c $(CFLAGS) -Iinclude -I$(PIXIE_SUITE_INC_DIR) $< -o $@

########################################################################

$(EXECUTABLE): $(OBJECTS)
ifneq ($(USE_SCANOR_READOUT), 1)
	$(LINKER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS)
else
	$(LINKER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS) $(SCANOR_OBJS)
endif

########################################################################

install: $(ALL_TOOLS)
	@echo " Installing to "$(INSTALL_DIR)
	@ln -s $(TOP_LEVEL)/run.sh $(INSTALL_DIR)/simpleScan

########################################################################

uninstall: tidy
	@rm -f $(INSTALL_DIR)/simpleScan

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
