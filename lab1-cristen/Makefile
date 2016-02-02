# Makefile. CS111 Lab 1a

CC = gcc
#CFLAGS =
DISTDIR=lab1-cristenKim
default: simpsh

simpsh: main.c 
	$(CC) main.c -o $@

dist: lab1-cristenKim.tar.gz

submission_files = README main.c Makefile test.sh execline a o e report.pdf
lab1-cristenKim.tar.gz: $(submission_files)
	tar -czf $@ --transform='s,^,$(DISTDIR)/,' $(submission_files)  

check: 
	./test.sh


clean:
	rm -f *.o *.tar.gz simpsh
