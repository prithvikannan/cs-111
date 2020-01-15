#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default:
	gcc -g -o lab0 -Wall -Wextra lab0.c

clean:
	rm -rf lab0 *.tar.gz

check: clean default
	bash ./checker.sh

dist: default
	tar -czvf lab0-405110096.tar.gz lab0.c README Makefile backtrace.png breakpoint.png checker.sh
