#ifndef SHELL
#define SHELL
#include <vector>
#include <map>
using namespace std;

typedef struct pipe_option_struct{ // store type |, !, >, and store length
        char opt;
        int len;
	int read_from; // store client id for user pipe read
	int write_to; // store client id for user pipe write
} pipe_option;

typedef struct pipe_linked_list{ // linked list struct
        int write_to_this; // = -1 means no other commands pipe to this
        int read_pipe; // = -1 means pipe to no commands
        struct pipe_linked_list *next;
} linked_list;

typedef struct ENV_CONTENT{
	char var[256];
	char val[256];
} env_content;

void child_handler(int signo); // SIGCHLD handler

class npshell {
private:
	linked_list *first;
	linked_list *last;
	//int slave_fd;
	vector<env_content> env_list;

public:
	//linked_list *first;
	//linked_list *last;
	int id;
	int slave_fd;
	char *ip;
	unsigned short port;
	char name[21];
	int pipe_read_from[30];
	int pipe_write_to[30];

	npshell(int ID, int fd, char *IP, unsigned short PORT);
	~npshell();

	void client_init();

	void client_recover();

	void parser(vector< vector<char*> > &arg, vector<pipe_option> &opt, char line[]); // parsing input from  stdin

	linked_list* linked_list_init(linked_list* &first, int size); // initialize linked list with fixed size

	void linked_list_new_node(linked_list* &first, linked_list* &last); // new one node

	linked_list* linked_list_traverse(linked_list* &first, int index); // return the index-th node

	int single_shell(bool need_sock, int sockfd, vector<npshell *> &client); // npshell for single process
};
#endif
