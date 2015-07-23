#!/bin/bash -e

DOC_CFG="doxygen.cfg"

# Check if doxy is visible
echo -n "Checking for doxygen... "
EXE_PATH=$(which doxygen)
if [ ! -x "$EXE_PATH" ] ; then
    echo "NOT FOUND, EXITING"
else
    echo "FOUND"
fi

# Change pwd to script dir, to keep the paths consistent
pushd . > /dev/null
cd "$(dirname "${BASH_SOURCE[0]}" )"

doxygen "$DOC_CFG"

popd > /dev/null
