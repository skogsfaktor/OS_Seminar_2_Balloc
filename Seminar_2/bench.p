#set terminal epslatex
set terminal png 
set output "bench.png"

set title "Balloc/Bfree Benchmark, 4 KiByte pages, 10 second operations"
set key right center
set xlabel "time in s"
set ylabel "allocated memory"
#set logscale x 2
plot "bench.dat" u 1:2 w linespoints title "mmap", "bench.dat" u 1:3 w linespoints title "balloc/bfree", "bench.dat" u 1:4 w linespoints title "allocated blocks"