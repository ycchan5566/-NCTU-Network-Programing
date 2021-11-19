#ifndef SHELL
#define SHELL
#include <vector>
using namespace std;

typedef struct pipe_option_struct{ // store type |, !, >, and store length
        char opt;
        int len;
} pipe_option;

typedef struct pipe_linked_list{ // linked list struct
        int write_to_this; // = -1 means no other commands pipe to this
        int read_pipe; // = -1 means pipe to no commands
        struct pipe_linked_list *next;
} linked_list;

void child_handler(int signo); // SIGCHLD handler

void parser(vector< vector<char*> > &arg, vector<pipe_option> &opt, char line[]); // parsing input from  stdin

linked_list* linked_list_init(linked_list* &first, int size); // initialize linked list with fixed size

void linked_list_new_node(linked_list* &first, linked_list* &last); // new one node

linked_list* linked_list_traverse(linked_list* &first, int index); // return the index-th node

int shell(); // npshell

#endif
