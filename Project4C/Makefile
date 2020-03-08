#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default:

dev: lab4c_tcp.c
	gcc -std=c99 -g -Wall -Wextra -lmraa lab4c_tcp.c -o lab4c_tcp -lm

clean:
	rm -rf *.o *.tar.gz lab4c_tcp lab4c_tls

dist: default
	tar -cvzf lab4c-204726167.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile
