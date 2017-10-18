#!/bin/bash
#
# Build a root data structure source file and header. This script
# Automatically builds the source files, header files, and objects
# required for building a root dictionary.
#
# Cory R. Thornsberry

TOP_LEVEL=`pwd`
DEF_FILE="./def.struct"
DICT_PREFIX="RootDict"

declare CLEAN_UP=0

# Check for passed arguments
while getopts 'hi:n:c' flag; do
  case "${flag}" in
    h) echo " SYNTAX:" ;
       echo "  ./rcbuild.sh <options>" ;
       echo "  Available Options:" ;
       echo "   -h | Show this dialogue." ;
       echo "   -i | Specify the definitions file." ;
       echo "   -n | Specify the name of the shared object file." ;
       echo "   -c | Clean up un-needed intermediate files." ;
       exit 0 ;;
    i) DEF_FILE="${OPTARG}" ;;
    n) DICT_PREFIX="${OPTARG}" ;;
    c) CLEAN_UP=1 ;;
    *) ;;
  esac
done

STRUCT_OBJ=Structures.o
STRUCT_SOURCE=Structures.cpp
STRUCT_HEADER=Structures.h
LINKFILE=LinkDef.h

BUILDER_EXE=rcbuild

DICT_SOURCE=${DICT_PREFIX}.cpp
DICT_OBJ=${DICT_PREFIX}.o
DICT_SHARED=lib${DICT_PREFIX}.so
DICT_STATIC=lib${DICT_PREFIX}.a
DICT_PCM_FILE=${DICT_PREFIX}_rdict.pcm

# Check for rcbuild
command -v ${BUILDER_EXE} >/dev/null 2>&1
RETVAL=$?

if [ ${RETVAL} -ne 0 ]; then
	echo " FATAL ERROR! "${BUILDER_EXE}" executable not found"
	exit 1
fi

# Rebuild the structure files
echo -n " [1/5] Generating data structure source files... "
$BUILDER_EXE -i $DEF_FILE
RETVAL=$?

# Check that the executable exited successfully
if [ ${RETVAL} -ne 0 ]; then
	echo "failed"
	echo "  FATAL ERROR! "$BUILDER_EXE" returned "$RETVAL
	exit $RETVAL
fi
echo "done"

# Compile the new structures file
echo -n " [2/5] Building data structures object file... "
g++ -Wall -fPIC -O2 `root-config --cflags` -c -o $STRUCT_OBJ $STRUCT_SOURCE
RETVAL=$?

# Check that g++ exited successfully
if [ ${RETVAL} -ne 0 ]; then
	echo "failed"
	echo "  FATAL ERROR! g++ (building "$STRUCT_OBJ") returned "$RETVAL
	exit $RETVAL
fi
echo "done"

#	Generate the dictionary source files using rootcint
echo -n " [3/5] Generating root dictionary source files... "
rootcint -f $DICT_SOURCE -c $STRUCT_HEADER $LINKFILE
RETVAL=$?

# Check that rootcint exited successfully
if [ ${RETVAL} -ne 0 ]; then
	echo "failed"
	echo "  FATAL ERROR! rootcint returned "$RETVAL
	exit $RETVAL
fi
echo "done"

# Compile rootcint source files
echo -n " [4/5] Building root dictionary object file... "
g++ -Wall -fPIC -O2 `root-config --cflags` -c -o $DICT_OBJ $DICT_SOURCE
RETVAL=$?

# Check that g++ exited successfully
if [ ${RETVAL} -ne 0 ]; then
	echo "failed"
	echo "  FATAL ERROR! g++ (building "$DICT_OBJ") returned "$RETVAL
	exit $RETVAL
fi
echo "done"

# Generate the root shared library (.so) for the dictionary
echo -n " [5/5] Building root libraries... "
g++ -g -shared -Wl,-soname,$DICT_SHARED -o $DICT_SHARED $STRUCT_OBJ $DICT_OBJ -lc
ar rcs $DICT_STATIC $STRUCT_OBJ $DICT_OBJ
RETVAL=$?

# Check that g++ exited successfully
if [ ${RETVAL} -ne 0 ]; then
	echo "failed"
	echo "  FATAL ERROR! g++ (building "$DICT_SHARED") returned "$RETVAL
	exit $RETVAL
fi
echo "done"

if [ ${CLEAN_UP} -eq 1 ]; then
	echo " Removing intermediate source files."
	rm -f ${DICT_OBJ} ${STRUCT_SOURCE} ${LINKFILE} ${DICT_SOURCE} ${DICT_OBJ}
fi

# Remove the root pcm file
rm -f ${DICT_PCM_FILE}

exit 0
