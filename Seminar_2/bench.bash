gcc -std=gnu99 -c buddy.c
gcc -std=gnu99 -o bench buddy.o bench.c
./bench
./bench > bench.dat
gnuplot bench.p