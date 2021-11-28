#!/bin/bash -e

declare -r MYDIR=$(cd $(dirname "$0") ; pwd )

cd "${MYDIR}"

VERSION=$(< ../../version)
cat pom.xml.in | sed "s/@HFTVERSION@/${VERSION}/g" > pom.xml
mvn clean install

mkdir -p export

cp -a target/hft-bridge-4.0.0.jar export/

find ~/.m2/repository/ -name "*.jar" | {
        while read line ; do
            bn=$(basename "$line")
            cp -va  "$line" export/
        done
}

##
## To start dukascopy bridge, goto:
##   cd export/
## then type:
##   java -cp  hft-bridge-4.0.0.jar:* hft2ducascopy/Main --config=<PATH_TO_HFT_CONFIG_XML>
##
