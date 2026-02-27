#!/bin/sh
#
# Finds specified string searchstr in files in specified filesDir file directory
#
# Arguments:
# filesdir - file directory to search
# searchstr - string to search files for
#
# Return values:
# 1 - error, message to std out

filesdir=$1
searchstr=$2

if [ $# -ne 2 ]
then
	echo "Incorrect number of arguments."
	echo "Two arguments required: (1) directory path, (2) search string"
	exit 1
fi

if [ -d ${filesdir} ]
then
	echo -n "The number of files are "
	grep -r -c ${searchstr} ${filesdir} | wc -l
	echo -n " and the number of matching lines are "
	grep -r -o -s ${searchstr} ${filesdir} | wc -l
else
	echo "The specified path is not a valid directory"
	exit 1
fi

