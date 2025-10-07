mkdir -p build
cd build

cmake .. && make && ./tasks/${1}/${1}

