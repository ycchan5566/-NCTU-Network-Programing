#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "shell.h"
using namespace std;


void child_handler(int signo){ // SIGCHLD handler
	int status;
	while(waitpid(-1, &status, WNOHANG) > 0){
		;
	}
}

void parser(vector< vector<char*> > &arg, vector<pipe_option> &opt, char line[]){ // type = 'a' means no pipe or stream
	vector<char*> temp_vector;
	for (char *tok=strtok(line, " \n\r"); tok != NULL; tok=strtok(NULL, " \n\r")){
		if (tok[0] == '|' || tok[0] == '!'){ // deal with '|' and '!'
			pipe_option opt_trans = { .opt = tok[0]};
			char number[5] = "1";
			for (int i=1; tok[i] != '\0'; i++){
				number[i-1] = tok[i];
			}
			opt_trans.len = atoi(number);
			opt.push_back(opt_trans);
			temp_vector.push_back(NULL);
			arg.push_back(temp_vector);
			temp_vector.clear();
			continue;
		}
		else if (tok[0] == '>'){ // deal with '>' 
			pipe_option opt_trans = { .opt = '>', .len = 0};	
			opt.push_back(opt_trans);
			temp_vector.push_back(NULL);
			tok=strtok(NULL, " \n\r");
			temp_vector.push_back(tok);
			arg.push_back(temp_vector);
			temp_vector.clear();
			continue;
		}
		temp_vector.push_back(tok);
	}
	if (temp_vector.size() > 0){ // deal with normal command
		pipe_option opt_trans = { .opt = 'a', .len = 0 };
		opt.push_back(opt_trans);
		temp_vector.push_back(NULL);
		arg.push_back(temp_vector);
	}
}

linked_list* linked_list_init(linked_list* &first, int size){ // initialize linked list with fixed size
	linked_list *now = first;
	for (int i=0; i<size; i++){
		linked_list *node;
		node = new linked_list[1];
		node->write_to_this = -1;
		node->read_pipe = -1;
		node->next = NULL;
		now->next = node;
		now = now->next;
	}

	return now;
}

void linked_list_new_node(linked_list* &first, linked_list* &last){ // new one node
	linked_list *node;
	node = new linked_list[1];
	node->write_to_this = -1;
	node->read_pipe = -1;
	node->next = NULL;

	last->next = node;
	last = last->next;
	node = first;
	first = first->next;
	delete node;
}

linked_list* linked_list_traverse(linked_list* &first, int index){ // return the index-th node
	linked_list *node = first;
	for (int i=0; i<index; i++){
		node = node->next;
	}

	return node;
}

int shell(bool need_sock, int sockfd){
char line[15000];
linked_list *first;
linked_list *last;
int stdin_int = 0;
int stdout_int = 1;
int stderr_int = 2;
FILE *stdin_file = stdin;

if (need_sock){
	stdin_int = sockfd;
	stdout_int = sockfd;
	stderr_int = sockfd;
	stdin_file = fdopen(sockfd, "r+");
}

first = new linked_list[1];
first->write_to_this = -1;
first->read_pipe = -1;

last = linked_list_init(first, 1001);

setenv("PATH", "bin:.", 1);
write(stdout_int, "% ", 2);

while(fgets(line, 15000, stdin_file)){

signal(SIGCHLD, child_handler);

vector<pipe_option> opt;
vector< vector<char *> > arg;

parser(arg, opt, line);

for (int idx = 0; idx<arg.size(); idx++){
	int fd_write = first->write_to_this;
	int fd_read = first->read_pipe;
	int fd_write_others = -1;
	int p[2]; // for pipe
	linked_list *node;
	pid_t pid;
	
	if (opt.at(idx).opt == '|' || opt.at(idx).opt == '!'){ // check whether the command needs to pipe
		node = linked_list_traverse(first, opt.at(idx).len);
		fd_write_others = node->write_to_this;

		if (fd_write_others == -1){
			if (pipe(p) < 0) cout << "pipe create error\n";
			node->write_to_this = p[1];
			node->read_pipe = p[0];
			fd_write_others = p[1];
		}
	}

	if (strcmp(arg.at(idx).at(0), "exit") == 0){ // built-in exit
		return 0;
	}

	else if (strcmp(arg.at(idx).at(0), "setenv") == 0){ // built-in setenv
		setenv(arg.at(idx).at(1), arg.at(idx).at(2), 1);
		linked_list_new_node(first, last);
		continue;
	}

	else if (strcmp(arg.at(idx).at(0), "printenv") == 0){ // built-in printenv
		if (getenv(arg.at(idx).at(1))){
			char *value = getenv(arg.at(idx).at(1));
			write(stdout_int, value, strlen(value));
			write(stdout_int, "\n", 1);
		}
		linked_list_new_node(first, last);
		continue;
	}
	
	if (fd_write != -1){
		close(fd_write);
	}

	while((pid = fork()) < 0){
		usleep(1000);
	}

	if (pid == 0){
		if (need_sock){
			dup2(sockfd, 0);
			dup2(sockfd, 1);
			dup2(sockfd, 2);
			close(sockfd);
		}

		if (fd_read != -1){
			dup2(fd_read, 0);
			close(fd_read);
		}
		if (fd_write_others != -1){
			dup2(fd_write_others, 1);
			if (opt.at(idx).opt == '!'){
				dup2(fd_write_others, 2);
			}
			close(fd_write_others);
		}
		if (opt.at(idx).opt == '>'){
			int file_fd = open(arg.at(idx).at(arg.at(idx).size()-1), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			dup2(file_fd, 1);
			close(file_fd);
		}
		execvp(arg.at(idx).at(0), arg.at(idx).data());
		fprintf(stderr, "Unknown command: [%s].\n", arg.at(idx).at(0));
		exit(0);
	}
	else if (pid > 0){

		if (idx == arg.size()-1){
			int status;
	        	if (opt.at(idx).opt=='>' || opt.at(idx).opt=='a') waitpid(pid, &status, 0);
		}
	}
	else {
		char *msg;
		msg = strerror(errno);
		cout << "Fork Error: " << msg << endl;
	}

	if (fd_read != -1){
		close(fd_read);
	}

	linked_list_new_node(first, last);	
}

write(stdout_int, "% ", 2);
}

return 0;
}
