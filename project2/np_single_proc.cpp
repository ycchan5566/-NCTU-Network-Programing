#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "include/single_shell.h"
using namespace std;

int main(int argc, char *argv[]){
	if (argc != 2){
		cout << "Invalid Argument" << endl;
		return 0;
	}

	struct sockaddr_in server_info, client_info;
	int sockfd, reuse_for_setsokopt = 1, slave_sock, maxfd = -1;
	int counter_for_binding=0; // for counting
	vector<npshell *> client(30, NULL);
	fd_set readfd;
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

	maxfd = sockfd;

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


	FD_ZERO(&readfd);
	FD_SET(sockfd, &readfd);
	cout << "\nServer starts!" << endl;
	while(1){
		fd_set copy = readfd;
		while(select(maxfd+1, &copy, NULL, NULL, NULL) < 1){
			cout << "Select Error!" << endl;
			copy = readfd;
		}
		for (int fd = 0; fd<maxfd+1; fd++){
		if (FD_ISSET(fd, &copy)){ // server accept
			if (fd == sockfd){
				if((slave_sock = accept(sockfd, (struct sockaddr*) &client_info, &addrlen)) == -1){
					cout << "Accept Error"<<endl;
				}
				if (slave_sock > maxfd){
					maxfd = slave_sock;
				}
				char welcome_msg[] = "****************************************\n** Welcome to the information server. **\n****************************************\n";
				write(slave_sock, welcome_msg, strlen(welcome_msg));

				int i;
				for (i=0; i<30; i++){
					if (client.at(i) == NULL){
						client.at(i) = new npshell(i, slave_sock, inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));
						client.at(i)->client_init();
						break;
					}
				}
				printf("New client {%s:%u} connects\n", client.at(i)->ip, client.at(i)->port);
				fflush(stdout);
				FD_SET(slave_sock, &readfd);
				for (int j=0; j<30; j++){
					if (client.at(j) != NULL){
						char login[100] = {0};
						snprintf(login, 100, "*** User '%s' entered from %s:%u. ***\n", client.at(i)->name, client.at(i)->ip, client.at(i)->port);
						write(client.at(j)->slave_fd, login, strlen(login));
					}
				}
				write(slave_sock, "% ", 2);
			}
		else { // client shell
			int i;
			for (i=0; i<30; i++){
				if(client.at(i) != NULL && client.at(i)->slave_fd == fd){
					break;
				}
			}

			client.at(i)->client_recover();

			switch(client.at(i)->single_shell(1, fd, client)){
				case 1: // exit
					FD_CLR(fd, &readfd);
					close(fd);
					for (int j=0; j<30; j++){
						if (client.at(j) != NULL && j != i){
							char logout[100] = {0};
							snprintf(logout, 100, "*** User '%s' left. ***\n", client.at(i)->name);
							write(client.at(j)->slave_fd, logout, strlen(logout));
						}
					}
					printf("Client '%s' has exit\n", client.at(i)->name);
					fflush(stdout);
					delete client.at(i);
					client.at(i) = NULL;
					for (int j=0; j<30; j++){
						if (client.at(j) != NULL){
							if (client.at(j)->pipe_write_to[i] != -1){
								close(client.at(j)->pipe_write_to[i]);
								client.at(j)->pipe_write_to[i] = -1;
							}
							if (client.at(j)->pipe_read_from[i] != -1){
								close(client.at(j)->pipe_read_from[i] != -1);
								client.at(j)->pipe_read_from[i] = -1;
							}
						}
					}
					break;
			}	
		}
		}
		}
	}
	close(sockfd);	
	return 0;
}
