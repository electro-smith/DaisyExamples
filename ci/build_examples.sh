#!/bin/bash
echo "building examples:"
start_dir=$PWD
example_dirs=( seed pod patch petal field versio patch_sm )
for e in ${example_dirs[@]}; do
    for d in $e/*/; do
        echo "building $d"
        cd "$d"
        make -s clean
        make -s
        if [ $? -ne 0 ]
        then
            echo "Failed to compile $d"
            exit 1
        fi
        cd "$start_dir"
        echo "done"
    done
done
