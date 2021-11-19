CC = g++
INC = -I ./include
LIB = -L ./include

npshell: main.o shell.o
	${CC} main.o shell.o -o npshell
main.o: main.cpp
	${CC} -c main.cpp
shell.o: include/shell.cpp
	${CC} -c include/shell.cpp
clean: 
	rm -rf *.o
	rm -f npshell
