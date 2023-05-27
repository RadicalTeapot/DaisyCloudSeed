#!/bin/bash
start_dir=$PWD

echo "Uploading to Daisy seed"
cd src
make program-dfu
echo "Done uploading"
cd "$start_dir"
