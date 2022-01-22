#!/bin/bash -e

PROJECT_TREE_DIRECTORY=$(dirname $0)/../

#
# Check if we have protobuf.
#

PROTOBUF_DIRECTORY="3rd-party/protobuf"
PROTOBUF_REPO="https://github.com/protocolbuffers/protobuf.git"

pushd $PROJECT_TREE_DIRECTORY > /dev/null

if [ ! -d "${PROTOBUF_DIRECTORY}" ] ; then
    pushd 3rd-party > /dev/null
    git clone $PROTOBUF_REPO
    cd protobuf
    git submodule update --init --recursive
    ./autogen.sh
    popd > /dev/null
fi

#
# Build requirements:
#  • autoconf
#  • automake
#  • libtool
#  • make
#  • g++
#

pushd 3rd-party/protobuf > /dev/null
    ./configure
    make
    make check
popd > /dev/null

popd > /dev/null