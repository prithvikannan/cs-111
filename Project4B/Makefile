#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: build
	
build: 
	gcc -g -lmraa -lm -Wall -Wextra -o lab4b lab4b.c

dev:
	gcc -g -D DEV=1 -lmraa -lm -Wall -Wextra -o lab4b lab4b.c

check: default

dist: README
	tar -czvf lab4b-405110096.tar.gz Makefile README lab4b.c

clean:
	rm -rf *.tar.gz