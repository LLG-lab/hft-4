#!/bin/bash -e
declare -r MYDIR=`cd $(dirname "$0") ; pwd`
cd $MYDIR
option=$1
data=$(< "${MYDIR}/../version")
if [ "x$option" = "x--major" ] ; then
    major=$(echo -n $data | awk -F'.' '{ print $1 }')
    echo -n $major
elif [ "x$option" = "x--minor" ] ; then
    minor=$(echo -n $data | awk -F'.' '{ print $2 }')
    echo -n $minor
else
    exit 1
fi