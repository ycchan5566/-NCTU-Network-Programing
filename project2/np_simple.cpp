#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "include/shell.h"
using namespace std;

int main(int argc, char *argv[]){
	if (argc != 2){
		cout << "Invalid Argument" << endl;
		return 0;
	}

	struct sockaddr_in server_info, client_info;
	int sockfd, reuse_for_setsokopt = 1, slave_sock;
	int counter_for_binding=0; // for counting
	uint16_t port = (uint16_t) atoi(argv[1]);

	cout << "Create socket..." << endl;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		cout << "Error while creating socket" << endl;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse_for_setsokopt, sizeof(int)) < 0){
		cout << "Error while setting SO_REUSEADDR" << endl;
	}
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void *) &reuse_for_setsokopt, sizeof(int)) < 0){
		cout << "Error while setting SO_REUSEPORT" << endl;
	}
	cout << "Finish creating socket\n" << endl;

  	socklen_t addrlen = sizeof(client_info);
	bzero(&server_info,sizeof(server_info));
	server_info.sin_family = PF_INET;
	server_info.sin_addr.s_addr = INADDR_ANY;
	server_info.sin_port = htons(port);

	cout << "Start binding..." << flush;
	while(bind(sockfd,(struct sockaddr *)&server_info,sizeof(server_info)) == -1){
		counter_for_binding++;
		if (counter_for_binding % 5000000 == 0) cout << '.' << flush;
	}
	cout << "\nFinish binding\n" << endl;

	cout << "Start listening..." << endl;
	while(listen(sockfd,30) == -1){
		;
	}
	cout << "Finish listening" << endl;

	cout << "\nServer starts!" << endl;
	while(1){
		while((slave_sock = accept(sockfd, (struct sockaddr*) &client_info, &addrlen)) == -1){
			cout << "Accept Error";
		}
		cout << "New client connects" << endl;
		shell(1, slave_sock);
		close(slave_sock);
		cout << "Client has exit\n" << endl;
	}
	close(sockfd);	
	return 0;
}
