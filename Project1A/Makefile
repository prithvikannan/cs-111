#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default:
	gcc -g -o lab1a -Wall -Wextra lab1a.c

clean:
	rm -rf lab1a *.tar.gz

check: clean default
	# signal=1 for ^C
	# signal=0 for ^D 
	# echo hello in bash

dist: default
	tar -czvf lab1a-405110096.tar.gz lab1a.c README Makefile
