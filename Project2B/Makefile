#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: build

build:
	gcc -g -lrt -pthread -Wall -Wextra SortedList.c -o lab2_list lab2_list.c

tests: default
	./lab2_tests.sh
	
graphs: clean tests
	./lab2_list.gp

clean:
	rm -rf lab2_add lab2_list *.o *.tar.gz

dist: clean graphs
	tar -czvf lab2b-405110096.tar.gz lab2_list.c SortedList.h SortedList.c Makefile README lab2_tests.sh *.gp *.csv *.png 
