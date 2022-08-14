#!/bin/bash
# Quick and dirty build script, needs to be in project root folder

cd Caby &&
mkdir -p build;
cd build;
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ../..;
cd Cacom;
cargo build --release
cd ..;
