#!/bin/bash
# Do an scp from a UT server

SCRIPT_FILENAME=run.sh

EXECUTABLE_NAME=./exec/PixieLDF

if [ "$#" -ge 3 ]
then
	# Change the output executable name.
	EXECUTABLE_NAME=$3
fi

if [ "$#" -ge 2 ]
then
	# Print some file header information.
	echo "#!/bin/bash" > $SCRIPT_FILENAME
	echo "# Setup a run script for executing the SimplePixieScan" >> $SCRIPT_FILENAME
	echo "# executable without having to set LD_LIBRARY_PATH." >> $SCRIPT_FILENAME
	echo "" >> $SCRIPT_FILENAME

	# Generate the line which sets the LD_LIBRARY_PATH.
	echo "LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:"$1":"$2" "$EXECUTABLE_NAME" \"\$@\"" >> $SCRIPT_FILENAME
	
	# Change the permissions of the script to allow execution.
	chmod a+x $SCRIPT_FILENAME
else
	echo " Invalid number of arguments"
	echo " SYNTAX: "$0" <pixie_suite_dir> <root_dict_dir> [executable_name]"
	
	exit 1
fi
