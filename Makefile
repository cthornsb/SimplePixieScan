#####################################################################

# Set the PixieSuite core directory
PIXIE_SUITE_DIR = $(HOME)/Research/Pixie16/PixieSuite

#####################################################################

#CFLAGS = -g -Wall -std=c++0x `root-config --cflags`
CFLAGS = -Wall -O3 -std=c++0x `root-config --cflags`
LDLIBS = -lstdc++ -L$(PIXIE_SUITE_DIR)/exec/lib -lPixieScan `root-config --libs`
LDFLAGS = `root-config --glibs`

COMPILER = g++

# Directories
TOP_LEVEL = $(shell pwd)

INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
OBJ_DIR = $(TOP_LEVEL)/obj

DICT_DIR = $(TOP_LEVEL)/dict
DICT_OBJ_DIR = $(DICT_DIR)/obj

RCBUILD_DIR = $(TOP_LEVEL)/rcbuild

# Core files
SOURCES = Plotter.cpp ProcessorHandler.cpp OnlineProcessor.cpp Processor.cpp ConfigFile.cpp MapFile.cpp Scanner.cpp

# Processors
SOURCES += TriggerProcessor.cpp \
           VandleProcessor.cpp \
           PhoswichProcessor.cpp \
           LiquidBarProcessor.cpp \
           NonwichProcessor.cpp \
           GenericProcessor.cpp \
           LogicProcessor.cpp

OBJECTS = $(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.o))

# ROOT dictionary stuff
DICT_SOURCE = RootDict
STRUCT_FILE_OBJ = $(OBJ_DIR)/Structures.o

ROOT_DICT = $(DICT_DIR)/$(DICT_SOURCE).cpp
ROOT_DICT_OBJ = $(DICT_OBJ_DIR)/$(DICT_SOURCE).o
ROOT_DICT_SLIB = $(DICT_OBJ_DIR)/$(DICT_SOURCE).so
SFLAGS = $(addprefix -l,$(DICT_SOURCE))

OBJECTS += $(ROOT_DICT_OBJ) $(STRUCT_FILE_OBJ)

EXECUTABLE = PixieLDF

########################################################################

all: directory dictionary $(EXECUTABLE)
#	Create all directories, make all objects, and link executable

dictionary:
#	Create root dictionary objects
	@$(RCBUILD_DIR)/rcbuild.sh
#	Copy the root .pcm file (if it exists) to the top directory
	@if [ -e $(DICT_DIR)/$(DICT_SOURCE)_rdict.pcm ]; then \
		cp $(DICT_DIR)/$(DICT_SOURCE)_rdict.pcm $(TOP_LEVEL)/; \
	fi

########################################################################

directory: $(OBJ_DIR)
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
	
########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) -c $(CFLAGS) -Iinclude -I$(PIXIE_SUITE_DIR)/exec/include $< -o $@

########################################################################

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS)

########################################################################

tidy: clean_obj clean_dict
	@rm -f $(EXECUTABLE)

clean: clean_obj

clean_obj:
	@echo "Cleaning up..."
	@rm -f $(OBJ_DIR)/*.o
	
clean_dict:
	@echo "Removing ROOT dictionaries..."
	@rm -f $(DICT_DIR)/$(DICT_SOURCE).cpp $(DICT_DIR)/$(DICT_SOURCE).h $(DICT_OBJ_DIR)/*.o  $(DICT_OBJ_DIR)/*.so
	@rm -f $(STRUCT_FILE_OBJ) $(SOURCE_DIR)/Structures.cpp $(INCLUDE_DIR)/Structures.h $(DICT_DIR)/LinkDef.h
