#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: server client

server:
	gcc -g -o lab1b-client -Wall -Wextra lab1b-client.c

client:
	gcc -g -o lab1b-server -Wall -Wextra lab1b-server.c

clean:
	rm -rf lab1b-client lab1b-server *.tar.gz

dist: default
	tar -czvf lab1b-405110096.tar.gz lab1b-client.c lab1b-server.c README Makefile
