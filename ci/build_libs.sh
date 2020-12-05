#!/bin/bash

START_DIR=$PWD
LIBDAISY_DIR=$PWD/libdaisy
DAISYSP_DIR=$PWD/DaisySP

echo "building libDaisy . . ."
cd $LIBDAISY_DIR ; make clean ; make | grep "warning:r\|error" ;
echo "done."

echo "building DaisySP . . ."
cd $DAISYSP_DIR ; make clean ; make | grep "warning:r\|error:" ;
echo "done."

