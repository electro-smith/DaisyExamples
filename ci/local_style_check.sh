#!/bin/bash

# TODO: 
# having to do a separate line for each exclude is silly
# and we should be able to run this with wildcards without it
# going crazy.

python ./utils/run-clang-format.py -r ./ \
    -e ./DaisySP -e ./libdaisy -e ./cube -e ./utils -e ./resources \
    -e ./seed/experimental -e ./patch/experimental \
    --extensions c,cpp,h
