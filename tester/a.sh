g++ -O2 -std=c++17 -Iinclude -Iinclude/crypto -Iinclude/tdutils -Iinclude/ton -Iinclude/common solution.cpp -L. -lton_crypto_lib -o solution

python3 run_tests.py