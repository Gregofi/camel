# A script for quick build and compile

if [[ "$#" -ne 1 ]]; then
    echo "usage: run.sh input-file"
    echo "  Quick and dirty script to quickly build both projects and execute the VM";
    echo "  Must be run from project root directory ('/camel')";
    exit 1;
fi


cd Caby &&
mkdir build;
cd build;
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ../..;
cd Cacom;
cargo build --release
cd ..;
./Cacom/target/release/cacom compile --input-file=${1} &&
./Caby/build/caby execute a.out;
rm a.out;