#!/bin/bash
start_dir=$PWD

echo "rebuilding source"
cd src
make clean | grep "warning\|error" # grep for silencing make outputs when regenerating everything.
make | grep "warning\|error"
echo "done building source"
cd "$start_dir"
