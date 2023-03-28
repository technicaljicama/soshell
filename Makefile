soshell: src/main.c
	gcc -Ofast -o soshell src/main.c
install: soshell
	cp soshell /usr/bin
clean: soshell
	rm -rf soshell
all: src/main.c
	gcc -Ofast -o soshell src/main.c
	cp soshell /usr/bin
	rm -rf soshell
