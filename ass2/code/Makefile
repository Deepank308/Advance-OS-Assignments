test_sfs: test_sfs.o disk.o sfs.o
	gcc -o test_sfs test_sfs.o disk.o sfs.o -Wall
test_sfs.o: test_sfs.c disk.h sfs.h
	gcc -c -g test_sfs.c -Wall

test_disk: test_disk.o disk.o sfs.o
	gcc -o test_disk test_disk.o disk.o sfs.o -Wall
test_disk.o: test_disk.c disk.h sfs.h
	gcc -c -g test_disk.c -Wall

main: main.o disk.o sfs.o
	gcc -o main main.o disk.o sfs.o -Wall
main.o: main.c disk.h sfs.h
	gcc -c -g main.c -Wall

sfs.o: sfs.c sfs.h
	gcc -c -g sfs.c -Wall
disk.o: disk.c disk.h
	gcc -c -g disk.c -Wall