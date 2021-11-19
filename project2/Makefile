CC = g++

out: object
	${CC} np_simple.o shell.o -o np_simple
	${CC} np_single_proc.o single_shell.o -o np_single_proc
	${CC} np_multi_proc.o multi_shell.o -o np_multi_proc
object: np_simple.cpp np_single_proc.cpp include/shell.cpp include/single_shell.cpp include/multi_shell.cpp
	${CC} -c np_simple.cpp
	${CC} -c np_single_proc.cpp
	${CC} -c np_multi_proc.cpp
	${CC} -c include/shell.cpp
	${CC} -c include/single_shell.cpp
	${CC} -c include/multi_shell.cpp
clean: 
	rm -rf *.o
	rm -f np_simple np_single_proc
