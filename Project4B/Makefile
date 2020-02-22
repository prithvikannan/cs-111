#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: prod
	
prod: 
	gcc -std=c99 -g -lmraa -lm -Wall -Wextra -o lab4b lab4b.c

dev:
	gcc -std=c99 -g -DDEV -lmraa -lm -Wall -Wextra -o lab4b lab4b.c

check: 

dist: README
	tar -czvf lab4b-405110096.tar.gz Makefile README lab4b.c

clean:
	rm -rf *.tar.gz