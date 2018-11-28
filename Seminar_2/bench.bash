gcc -std=gnu99 -c buddy.c -lm
gcc -std=gnu99 -o bench buddy.o bench.c -lm
./bench
./bench > bench.dat
gnuplot bench.p