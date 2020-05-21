#!/bin/bash
echo "rebuilding examples:"
start_dir=$PWD
HW=hardware_platforms
example_dirs=( daisysp/examples seed pod patch petal field )
for e in ${example_dirs[@]}; do
    for d in $e/*/; do
        echo "rebuilding $d"
        cd $d
        make clean | grep "warning\|error" # grep for silencing make outputs when regenerating everything.
        make | grep "warning\|error"
        cd $start_dir
        echo "done"
    done
done
