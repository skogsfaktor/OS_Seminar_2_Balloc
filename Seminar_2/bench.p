#set terminal epslatex
set terminal png 
set output "bench.png"

set title "Balloc/Bfree Benchmark, 4 KiByte pages, 100 operations"
set key right bottom
set xlabel "Total number of operations"
set ylabel "Allocated memory(bytes)"
#set logscale x 2
plot "bench.dat" u 1:2 w linespoints title "mmap", "bench.dat" u 1:3 w linespoints title "allocated memory", "bench.dat" u 1:4 w linespoints title "allocated blocks"