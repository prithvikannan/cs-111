#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default:
	gcc -g -o lab0 -Wall -Wextra project0.c

clean:
	rm -rf lab0 *.o *.txt *.tar.gz

check: clean default
	bash ./checkscript.sh

dist: default
	tar -czvf lab0-405110096.tar.gz project0.c README Makefile backtrace.PNG breakpoint.PNG

segfaultRun:
	./lab0 --segfault --catch
	echo $$?
	if [[ $$? -ne 4 ]]; then \
		echo "Incorrect exit code received for catching and handling segfault - should be 4" \
	fi
