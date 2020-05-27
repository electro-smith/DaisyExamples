#!/bin/bash
start_dir=$PWD

echo $start_dir

echo "rebuilding everything. . . "
echo "only errors, and warnings will output. . . "
echo "-------------------"
sleep 1

echo "rebuilding libdaisy"
cd libdaisy
make clean | grep "warningr\|error"
make | grep "warningr\|error"
echo "done"

echo "rebuilding DaisySP"
cd "$start_dir"
cd DaisySP
make clean | grep "warningr\|error"
make | grep "warningr\|error"
cd "$start_dir"
echo "done"

./rebuild_examples.sh

echo "finished"

