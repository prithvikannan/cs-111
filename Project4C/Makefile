#NAME: Prithvi Kannan
#EMAIL: prithvi.kannan@gmail.com
#ID: 405110096

.SILENT:

default: prod

prod:
	gcc -std=c99 -g -Wall -Wextra -lmraa lab4c_tcp.c -o lab4c_tcp -lm
	gcc -std=c99 -g -Wall -Wextra -lmraa -lcrypto -lssl lab4c_tls.c -o lab4c_tls -lm

dev: lab4c_tcp.c
	gcc -std=c99 -g -Wall -Wextra -lmraa -DDEV lab4c_tcp.c -o lab4c_tcp -lm
	gcc -std=c99 -g -Wall -Wextra -lmraa -lcrypto -lssl -DDEV lab4c_tls.c -o lab4c_tls -lm

clean:
	rm -rf *.o *.tar.gz lab4c_tcp lab4c_tls

dist: default
	tar -cvzf lab4c-405110096.tar.gz lab4c_tls.c lab4c_tcp.c README Makefile
