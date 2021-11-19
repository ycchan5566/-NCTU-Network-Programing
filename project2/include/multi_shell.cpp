#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <map>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "multi_shell.h"
using namespace std;

npshell *global_slave;


void child_handler(int signo){ // SIGCHLD handler
	int status;
	while(waitpid(-1, &status, WNOHANG) > 0){
		;
	}
}

void usr1_handler(int signo){ // for msg;
    pid_t who = getpid();
    int j;
    for(j=0; j<30; j++){
        if (global_slave[j].pid == who){
            break;
        }
    }
    write(global_slave[j].slave_fd, global_slave[j].msg, strlen(global_slave[j].msg));
}

void usr2_handler(int signo){
    pid_t who = getpid();
    int j;
    for(j=0; j<30; j++){
        if (global_slave[j].pid == who){
            break;
        }
    }
    write(global_slave[j].slave_fd, global_slave[j].recv_msg, strlen(global_slave[j].recv_msg));
}

npshell::npshell(int ID, int fd, char *IP, unsigned short PORT){
	id = ID;
	slave_fd = fd;
	snprintf(ip, 16, "%s", IP);
	port = PORT;
	strcpy(name, "(no name)");
    
    char welcome_msg[] = "****************************************\n** Welcome to the information server. **\n****************************************\n";
	write(slave_fd, welcome_msg, strlen(welcome_msg));


	/*for (int i=0; i<30; i++){
		pipe_read_from[i] = -1;
		pipe_write_to[i] = -1;
	}*/
}

npshell::~npshell(){	

	/*for (int i=0; i<30; i++){
		if (pipe_write_to[i] != -1){
			close(pipe_write_to[i]);
		}
		if (pipe_read_from[i] != -1){
			close(pipe_read_from[i]);
		}
	}*/
    close(slave_fd);
}

void npshell::client_init(){
	//init env var
	//clearenv();
	//setenv("PATH", "bin:.", 1);
	//env_content temp;
	//strcpy(temp.var, "PATH");
	//strcpy(temp.val, "bin:.");
	//env_list.push_back(temp);

	//init linked list
	//last = linked_list_init(first, 1001);
}

void npshell::client_recover(){
	//clearenv();
	//cout << "\nnew recover from \'" << name << "\' at \'" << ip << ":" << port <<"\'"<<endl;
	//for (int i=0; i < env_list.size(); i++){
	//	cout << env_list.at(i).var <<"  "<<env_list.at(i).val<<endl;
	//	setenv(env_list.at(i).var, env_list.at(i).val, 1);
	//}
}

void npshell::parser(vector< vector<char*> > &arg, vector<pipe_option> &opt, char line[]){ // type = 'a' means no pipe or stream
	vector<char*> temp_vector;
	pipe_option opt_trans;
	opt_trans.read_from = -1;
	opt_trans.write_to = -1;
	for (char *tok=strtok(line, " \n\r"); tok != NULL; tok=strtok(NULL, " \n\r")){
		if (!strcmp(tok, "tell")){
			temp_vector.push_back(tok);
			tok=strtok(NULL, " \n\r");
			temp_vector.push_back(tok);
			tok=strtok(NULL, "\n\r");
			char *i;
			for(i = tok; *i == ' '; i++);
			tok = i;
			temp_vector.push_back(tok);
			arg.push_back(temp_vector);
			temp_vector.clear();
			opt_trans.opt = 'a';
			opt_trans.len = 0;
			opt.push_back(opt_trans);
			break;
		}
		else if (!strcmp(tok, "yell")){
			temp_vector.push_back(tok);
			tok=strtok(NULL, "\n\r");
			char *i;
			for(i = tok; *i == ' '; i++);
			tok = i;
			temp_vector.push_back(tok);
			arg.push_back(temp_vector);
			temp_vector.clear();
			opt_trans.opt = 'a';
			opt_trans.len = 0;
			opt.push_back(opt_trans);
			break;
		}

		if (tok[0] == '|' || tok[0] == '!'){ // deal with '|' and '!'
			opt_trans.opt = tok[0];
			char number[5] = "1";
			for (int i=1; tok[i] != '\0'; i++){
				number[i-1] = tok[i];
			}
			opt_trans.len = atoi(number);
			opt.push_back(opt_trans);
			temp_vector.push_back(NULL);
			arg.push_back(temp_vector);
			temp_vector.clear();
			opt_trans.read_from = -1;
			opt_trans.write_to = -1;
			continue;
		}
		
		else if (tok[0] == '>' && tok[1] != '\0'){
			char number[3] = {0};
			for (int i=1; tok[i] != '\0'; i++){
				number[i-1] = tok[i];
			}
			opt_trans.write_to = atoi(number) - 1;
			cout << "user pipe write to id = " << opt_trans.write_to << endl;
			continue;
		}
			
		else if (tok[0] == '>'){ // deal with redirect 
			opt_trans.opt = 'f';
		       	opt_trans.len = 0;
			opt.push_back(opt_trans);
			temp_vector.push_back(NULL);
			tok=strtok(NULL, " \n\r");
			temp_vector.push_back(tok);
			arg.push_back(temp_vector);
			temp_vector.clear();
			opt_trans.read_from = -1;
			opt_trans.write_to = -1;
			continue;
		}

		else if (tok[0] == '<'){
			char number[3] = {0};
			for (int i=1; tok[i] != '\0'; i++){
				number[i-1] = tok[i];
			}
			opt_trans.read_from = atoi(number) - 1;
			cout << "user pipe read from id = " << opt_trans.read_from << endl;
			continue;
		}
		temp_vector.push_back(tok);
	}
	if (temp_vector.size() > 0){ // deal with normal command
		opt_trans.opt = 'a';
		opt_trans.len = 0;
		opt.push_back(opt_trans);
		temp_vector.push_back(NULL);
		arg.push_back(temp_vector);
	}
}

linked_list* npshell::linked_list_init(linked_list* &first, int size){ // initialize linked list with fixed size
	first = new linked_list[1];
	first->write_to_this = -1;
	first->read_pipe = -1;

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

void npshell::linked_list_new_node(linked_list* &first, linked_list* &last){ // new one node
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

linked_list* npshell::linked_list_traverse(linked_list* &first, int index){ // return the index-th node
	linked_list *node = first;
	for (int i=0; i<index; i++){
		node = node->next;
	}

	return node;
}

void linked_list_clean(linked_list* &first){
    linked_list *now = first;
	while(now != NULL){
		now = now->next;
		delete first;
		first = now;
	}
}

int npshell::multi_shell(bool need_sock, int sockfd, npshell* &slave){ // npshell for single process
global_slave = slave;

char line[15000];
linked_list *first;
linked_list *last;
int stdin_int = 0;
int stdout_int = 1;
int stderr_int = 2;
FILE *stdin_file = stdin;

last = linked_list_init(first, 1001);

clearenv();
setenv("PATH", "bin:.", 1);

if (need_sock){
	stdin_int = sockfd;
	stdout_int = sockfd;
	stderr_int = sockfd;
	stdin_file = fdopen(sockfd, "r+");
}

signal(SIGUSR1, usr1_handler);
signal(SIGUSR2, usr2_handler);

for (int j=0; j<30; j++){
    if (slave[j].id != -1){
    char login[100] = {0};
		snprintf(login, 100, "*** User '%s' entered from %s:%u. ***\n", name, ip, port);
		//write(client.at(j)->slave_fd, login, strlen(login));
        snprintf(slave[j].msg, 1500, "%s", login);
        kill(slave[j].pid, SIGUSR1);
	}
}

write(stdout_int, "% ", 2);

while (fgets(line, 15000, stdin_file)){

char command[15000] = {0};
strcpy(command, line);
for (char *tok=strtok(command, "\n\r"); tok != NULL; tok=strtok(NULL, "\n\r"));

signal(SIGCHLD, child_handler);
signal(SIGUSR1, usr1_handler);
signal(SIGUSR2, usr2_handler);
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
			while(pipe(p) < 0) cout << "pipe create error\n";
			node->write_to_this = p[1];
			node->read_pipe = p[0];
			fd_write_others = p[1];
		}
	}

	if (strcmp(arg.at(idx).at(0), "exit") == 0){ // built-in exit
        slave[id].id = -1;
        for (int j=0; j<30; j++){
            if (slave[j].id != -1 && j != id){
                char logout[100] = {0};
                snprintf(logout, 100, "*** User '%s' left. ***\n", name);
                //write(client.at(j)->slave_fd, logout, strlen(logout));
                snprintf(slave[j].msg, 1500, "%s", logout);
                kill(slave[j].pid, SIGUSR1);
            }
			/*for (int j=0; j<30; j++){
			    if (slave[j].id != -1){
			        if (slave[j].pipe_write_to[i] != -1){
				        close(client.at(j)->pipe_write_to[i]);
			            client.at(j)->pipe_write_to[i] = -1;
				}
				if (slave[j].pipe_read_from[i] != -1){
					close(client.at(j)->pipe_read_from[i] != -1);
					client.at(j)->pipe_read_from[i] = -1;
				    }
			    }
			}*/
        }
        for (int j=0; j<30; j++){
			    if (slave[j].id != -1){
			        if (slave[j].pipe_write_to[id] != -1){
				        //close(client.at(j)->pipe_write_to[i]);
			            slave[j].pipe_write_to[id] = -1;
				}
				if (slave[j].pipe_read_from[id] != -1){
					//close(client.at(j)->pipe_read_from[i] != -1);
					slave[j].pipe_read_from[id] = -1;
				    }
			    }
			}

        linked_list_clean(first);

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

	else if (strcmp(arg.at(idx).at(0), "who") == 0){ // who
		char first_line[] = "<ID>\t<nickname>\t<IP:port>\t<indicate me>\n";
		write(stdout_int, first_line, strlen(first_line));
		for (int i=0; i<30; i++){
			if (slave[i].id != -1){
				char info[200];
				if (id == i){
					snprintf(info, 200, "%d\t%s\t%s:%u\t<-me\n", i+1, slave[i].name, slave[i].ip, slave[i].port);
				}
				else{
					snprintf(info, 200, "%d\t%s\t%s:%u\t\n", i+1, slave[i].name, slave[i].ip, slave[i].port);
				}
				write(stdout_int, info, strlen(info));
			}
		}
		linked_list_new_node(first, last);
		continue;
	}

	else if (strcmp(arg.at(idx).at(0), "tell") == 0){ // tell
		int tell_to = atoi(arg.at(idx).at(1)) - 1;
		if (slave[tell_to].id == -1){
			char info[100];
			snprintf(info, 100, "*** Error: user #%d does not exist yet. ***\n", tell_to+1);
			write(stderr_int, info, strlen(info));
		}
		else{
			snprintf(slave[tell_to].msg, 1500, "*** %s told you ***: %s\n", name, arg.at(idx).at(2));
			//write(slave[tell_to].slave_fd, info, strlen(info));
            kill(slave[tell_to].pid, SIGUSR1);
		}
		linked_list_new_node(first, last);
		continue;
	}

	 else if (strcmp(arg.at(idx).at(0), "yell") == 0){ //yell
		 for (int i=0; i<30; i++){
			 if (slave[i].id != -1){
				 snprintf(slave[i].msg, 1500, "*** %s yelled ***: %s\n", name, arg.at(idx).at(1));
				 //write(client.at(i)->slave_fd, msg, strlen(msg));
                 kill(slave[i].pid, SIGUSR1);
			 }
		 }
		 linked_list_new_node(first, last);
		 continue;
	 }

	else if (strcmp(arg.at(idx).at(0), "name") == 0){ //name
		int i;
		for (i=0; i<30; i++){
			if (slave[i].id != -1){
				if (!strcmp(arg.at(idx).at(1), slave[i].name)){
					char info[100];
					snprintf(info, 100, "*** User '%s' already exists. ***\n", arg.at(idx).at(1));
					write(stdout_int, info, strlen(info));
					break;
				}
			}
		}
		if (i == 30){
			snprintf(name, 21, "%s", arg.at(idx).at(1));
            snprintf(slave[id].name, 21, "%s", arg.at(idx).at(1));
			char info[100];
			snprintf(info, 100, "*** User from %s:%u is named '%s'. ***\n", ip, port, name);
			for (i = 0; i<30; i++){
				if (slave[i].id != -1){
					//write(client.at(i)->slave_fd, info, strlen(info));
                    snprintf(slave[i].msg, 1500, "%s", info);
                    kill(slave[i].pid, SIGUSR1);
				}
			}
		}
		linked_list_new_node(first, last);
		continue;
	}
	
	if (fd_write != -1){
		close(fd_write);
	}

	bool close_user_pipe = 0;
	bool read_err = 0;
	bool write_err = 0;
	bool send_not_exist = 0;
	bool recv_not_exist = 0;
	int send_pipe = -1;
	int recv_pipe = -1;

	if (opt.at(idx).read_from != -1){
		if (slave[opt.at(idx).read_from].id != -1){
			 if (slave[opt.at(idx).read_from].pipe_write_to[id] != -1){
				close_user_pipe = 1;
                char fifo[100];
                snprintf(fifo, 100, "user_pipe/%dto%d", slave[opt.at(idx).read_from].id, id);
				recv_pipe = open(fifo, O_RDONLY);
				char info[1500];
				snprintf(info, 1500, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", name, id+1, slave[opt.at(idx).read_from].name, slave[opt.at(idx).read_from].id+1, command);
				for (int i = 0; i<30; i++){
					 if (slave[i].id  != -1){
						 //write(client.at(i)->slave_fd, info, strlen(info));
                         snprintf(slave[i].recv_msg, 1500, "%s", info);
                         kill(slave[i].pid, SIGUSR2);
					 }
				}
				//close(slave[opt.at(idx).read_from].pipe_write_to[id]);
				slave[opt.at(idx).read_from].pipe_write_to[id] = -1;
			 }
			 else {
				 read_err = 1;
				 if (opt.at(idx).opt != '!'){
					char info[200];
					snprintf(info, 200, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", opt.at(idx).read_from+1, id+1);
					write(stderr_int, info, strlen(info));
				 }
			 }
		}
		else {
			recv_not_exist = 1;
			if (opt.at(idx).opt != '!'){
				char info[200];
				snprintf(info, 200, "*** Error: user #%d does not exist yet. ***\n", opt.at(idx).read_from+1);
				write(stderr_int, info, strlen(info));
			}
		}
	}

    if (close_user_pipe && opt.at(idx).write_to != -1){
        usleep(1000);
    }

	if (opt.at(idx).write_to != -1){
		if (slave[opt.at(idx).write_to].id != -1){
			if (slave[opt.at(idx).write_to].pipe_read_from[id] == -1){
				char fifo[100];
                snprintf(fifo, 100, "user_pipe/%dto%d", id, slave[opt.at(idx).write_to].id);
				while(!(mkfifo(fifo, 0644) >= 0 || errno == EEXIST)) cout << "fifo create error\n";
				slave[id].pipe_write_to[opt.at(idx).write_to] = 1;
				slave[opt.at(idx).write_to].pipe_read_from[id] = 1;
				//send_pipe = open(fifo, O_WRONLY);
				char info[500];
				snprintf(info, 500, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", name, id+1, command, slave[opt.at(idx).write_to].name, slave[opt.at(idx).write_to].id+1);
				for (int i = 0; i<30; i++){
					if (slave[i].id != -1){
						//write(client.at(i)->slave_fd, info, strlen(info));
                        snprintf(slave[i].msg, 1500, "%s", info);
                        kill(slave[i].pid, SIGUSR1);
					}
				}
			}
			else{
				write_err = 1;
				if (opt.at(idx).opt != '!'){
					char info[200];
					snprintf(info, 200, "*** Error: the pipe #%d->#%d already exists. ***\n", id+1, opt.at(idx).write_to+1);
					write(stderr_int, info, strlen(info));
				}
			}
		}
		else {
			send_not_exist = 1;
			if (opt.at(idx).opt != '!'){
				char info[200];
				snprintf(info, 200, "*** Error: user #%d does not exist yet. ***\n", opt.at(idx).write_to+1);
				write(stderr_int, info, strlen(info));
			}
		}
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
		if (opt.at(idx).opt == 'f'){
			int file_fd = open(arg.at(idx).at(arg.at(idx).size()-1), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			dup2(file_fd, 1);
			close(file_fd);
		}

		if (opt.at(idx).read_from != -1){
			int dev_null = open("/dev/null", O_RDWR);
			if (recv_not_exist){
				if (opt.at(idx).opt == '!'){
				char info[200];
				snprintf(info, 200, "*** Error: user #%d does not exist yet. ***\n", opt.at(idx).read_from+1);
				write(2, info, strlen(info));
				}
				dup2(dev_null, 0);
			}
			else if (read_err){
				if (opt.at(idx).opt == '!'){
				char info[200];
				snprintf(info, 200, "*** Error: the pipe #%d->#%d does not exist yet. ***\n", opt.at(idx).read_from+1, id+1);
				write(2, info, strlen(info));
				}
				dup2(dev_null, 0);
			}
			else {
				dup2(recv_pipe, 0);
				close(recv_pipe);
			}
		}

		if (opt.at(idx).write_to != -1){
			int dev_null = open("/dev/null", O_RDWR);
			if (send_not_exist){
				if (opt.at(idx).opt == '!'){
				char info[200];
				snprintf(info, 200, "*** Error: user #%d does not exist yet. ***\n", opt.at(idx).write_to+1);
				write(2, info, strlen(info));
				}
				dup2(dev_null, 1);
			}
			else if (write_err){
				if (opt.at(idx).opt == '!'){
				char info[200];
				snprintf(info, 200, "*** Error: the pipe #%d->#%d already exists. ***\n", id+1, opt.at(idx).write_to+1);
				write(2, info, strlen(info));
				}
				dup2(dev_null, 1);
			}
			else {
                char fifo[100];
                snprintf(fifo, 100, "user_pipe/%dto%d", id, slave[opt.at(idx).write_to].id);
				send_pipe = open(fifo, O_WRONLY);
				dup2(send_pipe, 1);
				close(send_pipe);
			}
		}

		execvp(arg.at(idx).at(0), arg.at(idx).data());
		fprintf(stderr, "Unknown command: [%s].\n", arg.at(idx).at(0));
		exit(0);
	}
	else if (pid > 0){

		if (idx == arg.size()-1){
			int status;
	        	if ((opt.at(idx).opt=='f' || opt.at(idx).opt=='a') && opt.at(idx).write_to == -1) waitpid(pid, &status, 0);
		}
	}
	else {
		char *msg;
		msg = strerror(errno);
		cout << "Fork Error: " << msg << endl;
	}

	if (close_user_pipe){
		close(recv_pipe);
		slave[id].pipe_read_from[opt.at(idx).read_from] = -1;
	}

    if (send_pipe != -1){
        close(send_pipe);
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

