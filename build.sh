export MAKEFLAGS=-j$(($(grep -c "^processor" /proc/cpuinfo) - 1))

cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1

cmake --build build -j 20 #${MAKEFLAGS}