CC = g++

cgi: console.cpp
	${CC} -std=c++11 console.cpp -lboost_system -lpthread -o console.cgi
	${CC} -std=c++11 http_server.cpp -lboost_system -lpthread -o http_server
clean:
	rm -f console.cgi
	rm -f http_server
