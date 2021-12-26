#!/bin/bash -e

function error()
{
    local ERROR_MESSAGE="$1"

    if [ -t 1 ] ; then
        echo -e "\033[;31m ** ERROR! **\033[0m $ERROR_MESSAGE" >&2
    else
        echo "** ERROR! ** $ERROR_MESSAGE" >&2
    fi

    exit 1
}

function exists_generator_docker_image()
{
    docker --version > /dev/null 2>&1 || error "You have to install docker utility"

    local TEST=$(docker images | grep hft4-rpm-generator-centos7 | awk '{ print $1 }')

    if [ "x$TEST" = "xhft4-rpm-generator-centos7" ] ; then
        return 0
    fi

    return 1
}

###############
## Main Exec
###############

declare -r SELF_DIR=$(D=$(dirname "$0") ; cd "$D" ; pwd)
declare -r OPTION="$1"
INSPECT=no

if [ "x$OPTION" = "x--help" ] || [ "x$OPTION" = "x-h" ] ; then
    echo ""
    echo "RPM-GEN - Centos 7 distribution RPM packages generator for HFT"
    echo "Copytight (c) 2017 - 2022 by LLG Ryszard Gradowski, All Rights Reserved"
    echo
    echo "Usage:"
    echo "   $(basename $0) [-h|--help|--inspect]"
    echo ""

    exit 0
elif [ "x$OPTION" = "x--inspect" ] ; then
    INSPECT=yes
elif [ "x$OPTION" != "x" ] ; then
    error "Illegal option ‘$OPTION’. Type ‘$(basename $0) --help’ for more informations."
fi

cd "$SELF_DIR"

if ! exists_generator_docker_image ; then
    echo "Docker image ‘hft4-rpm-generator-centos7’ must be generated, now proceeding..."
    pushd hft-rpmbuild-dockerimage-src > /dev/null
    docker build -t hft4-rpm-generator-centos7 . || error "Failed to generate docker image ‘hft4-rpm-generator-centos7’"
    popd > /dev/null
fi

## Put project into exchange volume.
pushd ../ > /dev/null
tar -cf ${SELF_DIR}/exchange/SOURCES/hft-project.tar --exclude-from=.gitignore *
popd > /dev/null

## Start job inside docker container.

if [ $INSPECT = "no" ] ; then
    docker run --rm -v "${SELF_DIR}/exchange:/root/exchange" hft4-rpm-generator-centos7 /root/rpm-generator
elif [ $INSPECT = "yes" ] ; then 
    echo ""
    echo "Run manually ‘/root/rpm-generator’, where"
    echo ""
    docker run --rm -it -v "${SELF_DIR}/exchange:/root/exchange" hft4-rpm-generator-centos7
else
    error "Logic error. Unrecognized value : ${INSPECT}"
fi
