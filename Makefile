#####################################################################

# Set the PixieSuite directory
PIXIE_SUITE_DIR = /home/pixie16/cthorns/PixieSuitePLD

# Set the RootPixieScan directory
PIXIE_SCAN_DIR = /home/pixie16/cthorns/RootPixieScan

#####################################################################

CFLAGS = -g -Wall -O3 -std=c++0x `root-config --cflags`
LDLIBS = -lstdc++ `root-config --libs`
LDFLAGS = `root-config --glibs`

COMPILER = g++

# Directories
TOP_LEVEL = $(shell pwd)

INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
OBJ_DIR = $(TOP_LEVEL)/obj

DICT_DIR = $(PIXIE_SCAN_DIR)/dict
DICT_OBJ_DIR = $(DICT_DIR)/obj

SCAN_INC_DIR = $(PIXIE_SCAN_DIR)/include
SCAN_SRC_DIR = $(PIXIE_SCAN_DIR)/src
SCAN_OBJ_DIR = $(PIXIE_SCAN_DIR)/obj/c++

# Core files
SOURCES = ParentClass.cpp ProcessorHandler.cpp Processor.cpp ChannelEvent.cpp ConfigFile.cpp MapFile.cpp Unpacker.cpp

# Processors
SOURCES += TriggerProcessor.cpp \
           VandleProcessor.cpp \
           GenericProcessor.cpp

OBJECTS = $(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.o))

POLL_INC_DIR = $(PIXIE_SUITE_DIR)/Poll/include
POLL_SRC_DIR = $(PIXIE_SUITE_DIR)/Poll/source

# This is a special object file included from PixieSuite
HRIBF_SOURCE = $(POLL_SRC_DIR)/hribf_buffers.cpp
HRIBF_SOURCE_OBJ = $(OBJ_DIR)/hribf_buffers.o

# This is a special object file included from PixieSuite
SOCKET_SOURCE = $(POLL_SRC_DIR)/poll2_socket.cpp
SOCKET_SOURCE_OBJ = $(OBJ_DIR)/poll2_socket.o

SCAN_MAIN = $(SCAN_SRC_DIR)/ScanMain.cpp
SCAN_MAIN_OBJ = $(OBJ_DIR)/ScanMain.o

OBJECTS += $(HRIBF_SOURCE_OBJ) $(SOCKET_SOURCE_OBJ) $(SCAN_MAIN_OBJ)

# ROOT dictionary stuff
DICT_SOURCE = RootDict
STRUCT_HEAD = $(SCAN_INC_DIR)/Structures.h
STRUCT_FILE = $(SCAN_SRC_DIR)/Structures.cpp
STRUCT_FILE_OBJ = $(SCAN_OBJ_DIR)/Structures.o

ROOT_DICT = $(DICT_DIR)/$(DICT_SOURCE).cpp
ROOT_DICT_OBJ = $(DICT_OBJ_DIR)/$(DICT_SOURCE).o 
ROOT_DICT_SLIB = $(DICT_OBJ_DIR)/$(DICT_SOURCE).so 
SFLAGS = $(addprefix -l,$(DICT_SOURCE))

OBJECTS += $(ROOT_DICT_OBJ) $(STRUCT_FILE_OBJ)

EXECUTABLE = SimpleLDF

########################################################################

all: directory $(ROOT_DICT_SLIB) $(EXECUTABLE)

dictionary: $(DICT_OBJ_DIR) $(ROOT_DICT_SLIB)
#	Create root dictionary objects

.SECONDARY: $(DICT_DIR)/$(DICT_SOURCE).cpp $(ROOTOBJ)
#	Want to keep the source files created by rootcint after compilation
#	as well as keeping the object file made from those source files

########################################################################

directory: $(OBJ_DIR) $(DICT_OBJ_DIR)
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
	
$(DICT_OBJ_DIR):
#	Make root dictionary object file directory
	@if [ ! -d $@ ]; then \
		echo "Making directory: "$@; \
		mkdir $@; \
	fi

########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) -c $(CFLAGS) -Iinclude -I$(SCAN_INC_DIR) $< -o $@

$(HRIBF_SOURCE_OBJ): $(HRIBF_SOURCE)
#	Compile hribf_buffers from PixieSuite
	$(COMPILER) -c $(CFLAGS) -I$(POLL_INC_DIR) $< -o $@

$(SOCKET_SOURCE_OBJ): $(SOCKET_SOURCE)
#	Compile poll2_socket from PixieSuite
	$(COMPILER) -c $(CFLAGS) -I$(POLL_INC_DIR) $< -o $@

$(SCAN_MAIN_OBJ): $(SCAN_MAIN)
#	Main scan function
	$(COMPILER) -c $(CFLAGS) -DSIMPLE_SCAN -Iinclude -I$(SCAN_INC_DIR) -I$(POLL_INC_DIR) $< -o $@
	
#####################################################################

$(STRUCT_FILE_OBJ): $(STRUCT_FILE)
#	Compile structures file
	$(COMPILER) -c $(CFLAGS) -I$(SCAN_INC_DIR) $< -o $@

$(ROOT_DICT_OBJ): $(ROOT_DICT)
#	Compile rootcint source files
	$(COMPILER) -c $(CFLAGS) $< -o $@

$(ROOT_DICT_SLIB): $(STRUCT_FILE_OBJ) $(ROOT_DICT_OBJ)
#	Generate the root shared library (.so) for the dictionary
	$(COMPILER) -g -shared -Wl,-soname,lib$(DICT_SOURCE).so -o $(DICT_OBJ_DIR)/lib$(DICT_SOURCE).so $(STRUCT_FILE_OBJ) $(ROOT_DICT_OBJ) -lc

$(ROOT_DICT): $(STRUCT_HEAD) $(DICT_DIR)/LinkDef.h
#	Generate the dictionary source files using rootcint
	@cd $(DICT_DIR); rootcint -f $@ -c $(STRUCT_HEAD) $(DICT_DIR)/LinkDef.h

########################################################################

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) $(LDFLAGS) $(OBJECTS) -L$(DICT_OBJ_DIR) $(SFLAGS) -o $@ $(LDLIBS)

########################################################################

clean:
	@echo "Cleaning up..."
	@rm -f $(OBJ_DIR)/*.o
	@rm -f $(EXECUTABLE)
	
clean_dict:
	@echo "Removing ROOT dictionaries..."
	@rm -f $(DICT_DIR)/$(DICT_SOURCE).cpp $(DICT_DIR)/$(DICT_SOURCE).h $(DICT_OBJ_DIR)/*.o  $(DICT_OBJ_DIR)/*.so
