soshell: src/main.c
	gcc -Ofast -o soshell src/main.c
install: soshell
	cp soshell /usr/bin
