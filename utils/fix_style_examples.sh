#!/bin/bash
#TODO:
# - fix all the hard-coding
STARTDIR="../"
find $STARTDIR -iname '*.cpp' -type f -exec sed -i 's/\t/    /g' {} +
clang-format-6.0 -i $STARTDIR/pod/*/*.cpp $STARTDIR/seed/*/*.cpp
clang-format-6.0 -i $STARTDIR/patch/*/*.cpp $STARTDIR/petal/*/*.cpp $STARTDIR/field/*/*.cpp
