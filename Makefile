#####################################################################

# Set the PixieSuite directory
PIXIE_SUITE_DIR = /home/cory/Research/Pixie16/PixieSuite

#####################################################################

#CFLAGS = -g -Wall -O3 -std=c++0x `root-config --cflags` -Iinclude
#LDLIBS = -lstdc++ `root-config --libs`
#LDFLAGS = `root-config --glibs`

CFLAGS = -g -Wall -O3 -std=c++0x -Iinclude
LDLIBS = -lstdc++
LDFLAGS =

COMPILER = g++

# Directories
TOP_LEVEL = $(shell pwd)
DICT_DIR = $(TOP_LEVEL)/dict
INCLUDE_DIR = $(TOP_LEVEL)/include
SOURCE_DIR = $(TOP_LEVEL)/source
OBJ_DIR = $(TOP_LEVEL)/obj

# Core files
SOURCES = ParentClass.cpp ProcessorHandler.cpp Processor.cpp ChannelEvent.cpp ConfigFile.cpp MapFile.cpp Unpacker.cpp

# Processors
SOURCES += TriggerProcessor.cpp

OBJECTS = $(addprefix $(OBJ_DIR)/,$(SOURCES:.cpp=.o))

POLL_INC_DIR = $(PIXIE_SUITE_DIR)/Poll/include
POLL_SRC_DIR = $(PIXIE_SUITE_DIR)/Poll/source

# This is a special object file included from PixieSuite
HRIBF_SOURCE = $(POLL_SRC_DIR)/hribf_buffers.cpp
HRIBF_SOURCE_OBJ = $(OBJ_DIR)/hribf_buffers.o

# This is a special object file included from PixieSuite
SOCKET_SOURCE = $(POLL_SRC_DIR)/poll2_socket.cpp
SOCKET_SOURCE_OBJ = $(OBJ_DIR)/poll2_socket.o

# This is a special object file included from PixieSuite
#CTERMINAL_SOURCE = $(POLL_SRC_DIR)/CTerminal.cpp
#CTERMINAL_SOURCE_OBJ = $(OBJ_DIR)/CTerminal.o

SCAN_MAIN = $(SOURCE_DIR)/ScanMain.cpp
SCAN_MAIN_OBJ = $(OBJ_DIR)/ScanMain.o

OBJECTS += $(HRIBF_SOURCE_OBJ) $(SOCKET_SOURCE_OBJ) $(SCAN_MAIN_OBJ)

EXECUTABLE = SimpleLDF

########################################################################

all: $(EXECUTABLE)

########################################################################

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp
#	Compile C++ source files
	$(COMPILER) -c $(CFLAGS) $< -o $@

$(HRIBF_SOURCE_OBJ): $(HRIBF_SOURCE)
#	Compile hribf_buffers from PixieSuite
	$(CC) -c $(CFLAGS) -I$(POLL_INC_DIR) $< -o $@

$(SOCKET_SOURCE_OBJ): $(SOCKET_SOURCE)
#	Compile poll2_socket from PixieSuite
	$(CC) -c $(CFLAGS) -I$(POLL_INC_DIR) $< -o $@

$(CTERMINAL_SOURCE_OBJ): $(CTERMINAL_SOURCE)
#	Compile poll2_socket from PixieSuite
	$(CC) -c $(CFLAGS) -DUSE_NCURSES -I$(POLL_INC_DIR) -I$(INTERFACE_INC_DIR) $< -o $@

$(SCAN_MAIN_OBJ): $(SCAN_MAIN)
#	Main scan function
#	$(COMPILER) -c $(CFLAGS) -DUSE_NCURSES -I$(POLL_INC_DIR) $< -o $@
	$(COMPILER) -c $(CFLAGS) -I$(POLL_INC_DIR) $< -o $@

########################################################################

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER) $(LDFLAGS) $(OBJECTS) -o $@ $(LDLIBS)

########################################################################

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(EXECUTABLE)
