
.SILENT:

default:
	gcc -g -o project0 -Wall -Wextra project0.c

clean:
	rm -rf project0 *.o *.txt *.tar.gz

check: clean default successfulRun

dist: default
	tar -czvf project0-420.tar.gz project0.c README Makefile backtrace.png breakpoint.png

successfulRun:
	echo "420" > in.txt
	./project0 --input in.txt --output out.txt
	if [[ $$? -ne 0 ]]; then \
		echo "Incorrect exit code upon sucessfully getting through the program."; \
	fi

	diff in.txt out.txt
	if [[ $$? -ne 0 ]]; then \
		echo "in.txt does not match out.txt."; \
	fi

segfaultRun:
	./project0 --segfault --catch
	if [[ $$? -ne 4 ]]; then \
		echo "Incorrect exit code received for catching and handling segfault - should be 4" \
	fi
