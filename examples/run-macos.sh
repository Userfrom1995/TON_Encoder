# Usage: ./run-macos.sh solution.cpp
set -xe
g++ -O2 -std=c++17 -Iinclude -Iinclude/crypto -Iinclude/tdutils -Iinclude/ton -Iinclude/common "$1" -L. -lton_crypto_lib -o executable
DYLD_LIBRARY_PATH=. ./executable
