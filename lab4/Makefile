# Makefile for CS 111 Lab 4

CC = gcc
#CFLAGS =
DISTDIR=lab1-cristen
default: addtest

addtest: main.c 
	$(CC) main.c -o $@

dist: lab1-cristen.tar.gz

submission_files = answers.txt main.c Makefile test.sh report.pdf

lab1-cristen.tar.gz: $(submission_files)
	tar -czf $@ --transform='s,^,$(DISTDIR)/,' $(submission_files)  

check: 
	./test.sh

clean:
	rm -f *.o *.tar.gz simpsh