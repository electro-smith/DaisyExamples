#!/bin/bash
echo "building examples:"
start_dir=$PWD
example_dirs=( seed pod patch petal field versio patch_sm )

# Loop through the folders within each example dir
# ignoring folders containing the name, "experiemental"
# stop on a fail
for e in ${example_dirs[@]}; do
    for d in $e/*/; do
        case $d in
            *"experimental"*)
                ;;
            *)
                echo "building $d"
                cd $d
                make -s clean;
                make -s
                if [ $? -ne 0 ]; then
                    echo "Failed to compile $d"
                    exit 1
                fi
                cd "$start_dir"
                echo "done"
        esac

    done
done
