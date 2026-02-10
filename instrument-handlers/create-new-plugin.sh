#!/bin/bash -e

##############
## Main Exec
##############

declare -r MYDIR=$(cd $(dirname "$0") ; pwd )

if [ ${#@} -eq 0 ] ; then
    echo "HFT Instrument Handler src-code Creator v.1.0"
    echo "Copyright (c) 2017-2026 by LLG Ryszard Gradowski, All Rights Reserved"
    echo ""
    echo "Usage:"
    echo " ${0}  <new-plugin-name>"
    echo
    exit 0
fi

cd "${MYDIR}"

PLUGIN_NAME="$1"
PLUGIN_UPERCASE_NAME=${PLUGIN_NAME^^}

cp -a plugin_template ${PLUGIN_NAME}
sed -i "s/plugin_template/${PLUGIN_NAME}/g" ${PLUGIN_NAME}/CMakeLists.txt
mv ${PLUGIN_NAME}/plugin_template.cpp ${PLUGIN_NAME}/${PLUGIN_NAME}.cpp
mv ${PLUGIN_NAME}/include/plugin_template.hpp ${PLUGIN_NAME}/include/${PLUGIN_NAME}.hpp

sed -i "s/plugin_template/${PLUGIN_NAME}/g" ${PLUGIN_NAME}/include/${PLUGIN_NAME}.hpp
sed -i "s/PLUGIN_TEMPLATE/${PLUGIN_UPERCASE_NAME}/g" ${PLUGIN_NAME}/include/${PLUGIN_NAME}.hpp
sed -i "s/plugin_template/${PLUGIN_NAME}/g" ${PLUGIN_NAME}/${PLUGIN_NAME}.cpp

