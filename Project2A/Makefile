#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: build

build: lab2_add.c
	gcc -Wall -Wextra -g -o lab2_add -lpthread lab2_add.c

test:  

graphs: tests
	./lab2_add.gp
	./lab2_list.gp

clean:
	rm -f lab2_list lab2_add *.tar.gz

dist: default
	tar -czvf lab1b-405110096.tar.gz lab1b-client.c lab1b-server.c README Makefile
