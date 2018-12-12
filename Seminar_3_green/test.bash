gcc -c green.c -w
gcc -o test green.o test.c -w -pthread 
./test