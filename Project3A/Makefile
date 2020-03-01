#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: build

build: 
	gcc -g -Wall -Wextra -std=gnu99 lucas_lab3a.c -o lab3a

dist:
	tar -cvzf lab3a-405110096.tar.gz lab3a.c ext2_fs.h Makefile README

clean:
	rm -f lab3a *.tar.gz *_.csv