#!/bin/bash
#
# Build a root data structure source file and header. This script
# Automatically builds the source files, header files, and objects
# required for building a root dictionary.
#
# Cory R. Thornsberry

TOP_LEVEL=`pwd`
BUILD_DIR=
DICT_DIR=
SRC_DIR=
INC_DIR=
OBJ_DIR=

# Check for passed arguments
while getopts 'hc:b:d:s:i:o:' flag; do
  case "${flag}" in
    h) echo " SYNTAX:" ;
       echo "  ./rcbuild.sh <options> [directory]" ;
       echo "  Available Options:" ;
       echo "   -h | Show this dialogue" ;
       echo "   -c | Specify the parent directory" ;
       echo "   -b | Specify the rcbuild directory" ;
       echo "   -d | Specify the dict directory" ;
       echo "   -s | Specify the source directory" ;
       echo "   -i | Specify the include directory" ;
       echo "   -o | Specify the object directory" ;
       exit 0 ;;
    c) TOP_LEVEL="${OPTARG}" ;;
    b) BUILD_DIR="${OPTARG}" ;;
    d) DICT_DIR="${OPTARG}" ;;
    s) SRC_DIR="${OPTARG}" ;;
    i) INC_DIR="${OPTARG}" ;;
    o) OBJ_DIR="${OPTARG}" ;;
    *) ;;
  esac
done

if [ -z "$BUILD_DIR" ] ; then
	BUILD_DIR=$TOP_LEVEL/rcbuild
fi

if [ -z "$DICT_DIR" ] ; then
	DICT_DIR=$TOP_LEVEL/dict
fi

if [ -z "$SRC_DIR" ] ; then
	SRC_DIR=$TOP_LEVEL/source
fi

if [ -z "$INC_DIR" ] ; then
	INC_DIR=$TOP_LEVEL/include
fi

if [ -z "$OBJ_DIR" ] ; then
	OBJ_DIR=$TOP_LEVEL/obj
fi

DICT_OBJ_DIR=$DICT_DIR/obj

STRUCT_OBJ=Structures.o
STRUCT_SOURCE=Structures.cpp
STRUCT_HEADER=Structures.h
LINKFILE=LinkDef.h

BUILDER_EXE=rcbuild
BUILDER_SRC=rcbuild.cpp

DEF_FILE=$DICT_DIR/def.struct
DEF_FILE_BKP=$DICT_DIR/.def.struct.bkp

DICT_SOURCE=RootDict.cpp
DICT_OBJ=RootDict.o
DICT_SHARED=libRootDict.so

declare -i LEVEL_NEEDED=0
declare -i RETVAL=0

# Ensure that the definitions file exists
if [ ! -f $DEF_FILE ]; then
	echo " ERROR: Structure definition file does not exist!"

	if [ ! -f $DEF_FILE_BKP ]; then
		echo "  Searching for backup definition file... failed"
		echo "  FATAL ERROR! Failed to find backup file!"
		exit 1
	fi

	# Restore the backup of the definitions file
	echo "  Searching for backup definition file... done"
	cp -p $DEF_FILE_BKP $DEF_FILE
fi

# Make root dictionary object file directory if it doesn't already exist
if [ ! -d $DICT_OBJ_DIR ]; then \
	echo "Making directory: "$DICT_OBJ_DIR
	mkdir $DICT_OBJ_DIR
fi

# Determine what needs to be done
if [ $DEF_FILE -nt $DEF_FILE_BKP ]; then
	# Definitions file was modified, rebuild the entire file tree
	LEVEL_NEEDED=5
else
	# Definitions file was not modified, but check for missing files along the build tree
	if [ -f $SRC_DIR/$STRUCT_SOURCE ] && [ -f $INC_DIR/$STRUCT_HEADER ] && [ -f $DICT_DIR/$LINKFILE ]; then
		if [ -f $OBJ_DIR/$STRUCT_OBJ ]; then
			if [ -f $DICT_DIR/$DICT_SOURCE ]; then
				if [ -f $DICT_OBJ_DIR/$DICT_OBJ ]; then
					if [ -f $DICT_OBJ_DIR/$DICT_SHARED ]; then
						# All files exist. Nothing is needed.
						LEVEL_NEEDED=0
						echo " Nothing to be done for "$DICT_SHARED
						exit 0
					else
						# Need to build shared library libRootDict.so
						LEVEL_NEEDED=1
					fi
				else
					# Need to build RootDict.o
					LEVEL_NEEDED=2
				fi
			else
				# Need to use rootcint to generate RootDict.cpp
				LEVEL_NEEDED=3
			fi
		else
			# Need to build Structures.o
			LEVEL_NEEDED=4
		fi
	else
		# Full generation and compilation needed
		LEVEL_NEEDED=5
	fi
fi

if [ $LEVEL_NEEDED -ge 5 ]; then
	# Build the builder executable, if needed
	if [ ! -f $BUILD_DIR/$BUILDER_EXE ]; then
		echo -n " [1/6] Building structures file generator... "
		g++ -Wall -O2 -o $BUILD_DIR/$BUILDER_EXE $BUILD_DIR/build/$BUILDER_SRC
		RETVAL=$?

		# Check that the executable built successfully
		if [ $RETVAL -ne 0 ]; then
			echo "failed"
			echo "  FATAL ERROR! Failed to build "$BUILDER_EXE"!"
			exit $RETVAL
		fi
		echo "done"
	fi

	# Rebuild the structure files
	echo -n " [2/6] Generating data structure source files... "
	$BUILD_DIR/$BUILDER_EXE $DEF_FILE --src-dir $SRC_DIR/ --inc-dir $INC_DIR/ --dict-dir $DICT_DIR/
	RETVAL=$?

	# Check that the executable exited successfully
	if [ $RETVAL -ne 0 ]; then
		echo "failed"
		echo "  FATAL ERROR! "$BUILDER_EXE" returned "$RETVAL
		exit $RETVAL
	fi
	echo "done"
fi

if [ $LEVEL_NEEDED -ge 4 ]; then
	# Compile the new structures file
	echo -n " [3/6] Building data structures object file... "
	g++ -Wall -fPIC -O2 `root-config --cflags` -c -o $OBJ_DIR/$STRUCT_OBJ -I$INC_DIR $SRC_DIR/$STRUCT_SOURCE
	RETVAL=$?

	# Check that g++ exited successfully
	if [ $RETVAL -ne 0 ]; then
		echo "failed"
		echo "  FATAL ERROR! g++ (building "$STRUCT_OBJ") returned "$RETVAL
		exit $RETVAL
	fi
	echo "done"
fi

if [ $LEVEL_NEEDED -ge 3 ]; then
	#	Generate the dictionary source files using rootcint
	echo -n " [4/6] Generating root dictionary source files... "
	cd $DICT_DIR
	rootcint -f $DICT_DIR/$DICT_SOURCE -c $INC_DIR/$STRUCT_HEADER $DICT_DIR/$LINKFILE
	RETVAL=$?
	cd $TOP_LEVEL

	# Check that rootcint exited successfully
	if [ $RETVAL -ne 0 ]; then
		echo "failed"
		echo "  FATAL ERROR! rootcint returned "$RETVAL
		exit $RETVAL
	fi
	echo "done"
fi

if [ $LEVEL_NEEDED -ge 2 ]; then
	#	Compile rootcint source files
	echo -n " [5/6] Building root dictionary object file... "
	g++ -Wall -fPIC -O2 `root-config --cflags` -c -o $DICT_OBJ_DIR/$DICT_OBJ $DICT_DIR/$DICT_SOURCE
	RETVAL=$?

	# Check that g++ exited successfully
	if [ $RETVAL -ne 0 ]; then
		echo "failed"
		echo "  FATAL ERROR! g++ (building "$DICT_OBJ") returned "$RETVAL
		exit $RETVAL
	fi
	echo "done"
fi

if [ $LEVEL_NEEDED -ge 1 ]; then
	#	Generate the root shared library (.so) for the dictionary
	echo -n " [6/6] Building root shared library file... "
	g++ -g -shared -Wl,-soname,$DICT_SHARED -o $DICT_OBJ_DIR/$DICT_SHARED $OBJ_DIR/$STRUCT_OBJ $DICT_OBJ_DIR/$DICT_OBJ -lc
	RETVAL=$?

	# Check that g++ exited successfully
	if [ $RETVAL -ne 0 ]; then
		echo "failed"
		echo "  FATAL ERROR! g++ (building "$DICT_SHARED") returned "$RETVAL
		exit $RETVAL
	fi
	echo "done"
fi

# If everything exited correctly, make a backup of the definitions file
if [ $LEVEL_NEEDED -ge 5 ]; then
	cp -p $DEF_FILE $DEF_FILE_BKP
fi

exit 0
