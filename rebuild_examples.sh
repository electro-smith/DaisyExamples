#!/bin/bash
echo "rebuilding examples:"
start_dir=$PWD
HW=hardware_platforms
example_dirs=( daisysp/examples $HW/field/examples $HW/patch/examples $HW/petal/examples $HW/pod/examples $HW/seed/tests )
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
