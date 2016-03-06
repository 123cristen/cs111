# Makefile for CS 111 Lab 4

CC = gcc
CFLAGS = -pthread -lrt
DISTDIR=lab4-cristen
default: sltest 

addtest: main.c 
	$(CC) $(CFLAGS) main.c -o $@

sltest: list.c
	$(CC) $(CFLAGS) list.c -o $@

dist: lab4-cristen.tar.gz

submission_files = answers.txt main.c list.c Makefile test.sh report.pdf

lab4-cristen.tar.gz: $(submission_files)
	tar -czf $@ --transform='s,^,$(DISTDIR)/,' $(submission_files)  

check: 
	./test.sh

clean:
	rm -f *.o *.tar.gz addtest sltest