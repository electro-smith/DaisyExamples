#!/bin/bash

python ./libdaisy/ci/run-clang-format.py -r ./ -e ./DaisySP -e ./libdaisy -e ./cube -e ./utils -e ./resources --extensions c,cpp,h
