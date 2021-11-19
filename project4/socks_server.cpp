#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
using namespace std;

#define BUFFER_SIZE 150001

void child_handler(int signo){ // SIGCHLD handler
    int status;
    while(waitpid(-1, &status, WNOHANG) > 0){
        ;
    }
}

int connectDestHost(char dst_ip[INET_ADDRSTRLEN], unsigned short dst_port) {
    
    /**
     *	Get server info
     */
    int status;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    
    char port[6];
    sprintf(port, "%hu", dst_port);
    if ((status = getaddrinfo(dst_ip, port, &hints, &res)) != 0) {
        //log("getaddrinfo: " + string(gai_strerror(status)));
        return -1;
    }
    
    /**
     *	Get sockfd
     */
    int rsock = socket(AF_INET, SOCK_STREAM, 0);
    if (rsock == -1) {
        //log("Socket error: " + string(strerror(errno)));
        return -1;
    }
    
    /**
     *	Connect
     */
    if (connect(rsock, res->ai_addr, res->ai_addrlen) == -1) {
        //log("Connect error: " + string(strerror(errno)));
        return -1;
    }
    
    freeaddrinfo(res);
    return rsock;
}


void redirectData(int ssock, int rsock) {
    
    // use `select` to get data
    // more importantly, use two `select`s to separate `rfds` and `wfds`
    // or would result in infinite loop (select always returns 2, i.e. always can write)
    
    char buffer[BUFFER_SIZE+1];
    int nfds = FD_SETSIZE;
    fd_set rfds;    // readable fds used in select
    fd_set wfds;    // writable fds used in select
    fd_set rs;      // active read fds
    fd_set ws;      // actvie write fds
    
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&rs);
    FD_ZERO(&ws);
    
    FD_SET(ssock, &rs);
    FD_SET(ssock, &ws);
    FD_SET(rsock, &rs);
    FD_SET(rsock, &ws);
    
    char rsockbuffer[BUFFER_SIZE];
    char ssockbuffer[BUFFER_SIZE];
    size_t rsockbuffersize = 0;
    size_t ssockbuffersize = 0;
    bzero(rsockbuffer, BUFFER_SIZE);
    bzero(ssockbuffer, BUFFER_SIZE);

    bool rsockbufempty = true;
    bool ssockbufempty = true;
    
    struct timeval timeout;
    timeout.tv_sec = 1200;
    int toCheck = 0;
    
    while (1) {
        
        memcpy(&rfds, &rs, sizeof(rfds));
        memcpy(&wfds, &ws, sizeof(wfds));

        toCheck = select(nfds, &rfds, NULL, NULL, &timeout);
        if (toCheck == 0) {
            break;
        }

        if (ssockbufempty && FD_ISSET(ssock, &rfds)) {

            ssockbuffersize = read(ssock, buffer, BUFFER_SIZE+1);
            char* readStr = buffer;
            // manually strcpy
            for (int i = 0; i < ssockbuffersize; i++) {
                ssockbuffer[i] = readStr[i];
            }
            ssockbufempty = false;
//            strncpy(ssockbuffer, readStr, BUFFER_SIZE);
        }
        
        
        if (rsockbufempty && FD_ISSET(rsock, &rfds)) {

            rsockbuffersize = read(rsock, buffer, BUFFER_SIZE+1);
            char* readStr = buffer;
            // manually strcpy
            for (int i = 0; i < rsockbuffersize; i++) {
                rsockbuffer[i] = readStr[i];
            }
            rsockbufempty = false;
//            strncpy(rsockbuffer, readStr, BUFFER_SIZE);
        }
        
        toCheck = select(nfds, NULL, &wfds, NULL, &timeout);
        if (toCheck == 0) {
            break;
        }
        
        if (!ssockbufempty && FD_ISSET(rsock, &wfds)) {
            write(rsock, ssockbuffer, ssockbuffersize);
            bzero(ssockbuffer, BUFFER_SIZE);
            ssockbufempty = true;
        }

        
        if (!rsockbufempty && FD_ISSET(ssock, &wfds)) {
            write(ssock, rsockbuffer, rsockbuffersize);
            bzero(rsockbuffer, BUFFER_SIZE);
            rsockbufempty = true;
        }
    }


}
int main(int argc, char *argv[]){
    if (argc != 2){
		cout << "Usage: socks_server port" << endl;
		return 1;
	}

    // Server socket setting
    struct sockaddr_in server_info, client_info;
	int sockfd, reuse_for_setsokopt = 1, slave_sock;
	int counter_for_binding=0; // for counting
	int port = atoi(argv[1]);

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
	server_info.sin_family = AF_INET;
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

	cout << "\nServer starts!\n\n" << endl;
    // Finish server socket setting

	while(1){
		while((slave_sock = accept(sockfd, (struct sockaddr*) &client_info, &addrlen)) == -1){
			cout << "Accept Error";
		}

        char sip[20];
        unsigned short sport;
        snprintf(sip, 20, "%s", inet_ntoa(client_info.sin_addr));
        sport = ntohs(client_info.sin_port);
        signal(SIGCHLD, child_handler);
        /*struct sigaction signal_action;
        signal_action.sa_handler = SIG_DFL;
        signal_action.sa_flags = SA_NOCLDWAIT | SA_RESTART;
        sigaction(SIGCHLD, &signal_action, NULL);*/

        pid_t pid;
        while((pid = fork()) < 0){
            usleep(1000);
        }

        if (pid == 0){
            close(sockfd);
            char dip[20];
            int cd, dport;


            unsigned char buf[262] = {0};
            read(slave_sock, buf, 262);

            if ((int)buf[0] != 4){
                cout << "\nSOCKS 5" << endl;
                exit(0);
            }

            cd = (int)buf[1];
            dport = 256 * (int)buf[2] + (int)buf[3];
            snprintf(dip, 20, "%u.%u.%u.%u", buf[4], buf[5], buf[6], buf[7]);

            if (buf[4] == 0 && buf[5] == 0 && buf[6] == 0){
                int status;
                char *addr;
                char c_dport[6];
                struct addrinfo hints;
                struct addrinfo *name_to_ip;

                //for (int i=0; i<50; i++) cout << (int)buf[i] << " ";
                //cout << endl;

                memset(&hints, 0, sizeof hints); 
                hints.ai_family = AF_UNSPEC;
                hints.ai_socktype = SOCK_STREAM;

                int i;
                for (i=8; buf[i] != 0; i++);
                addr = strtok((char *)&buf[i+1], "\0");
                //cout << "addr = " << addr << endl;
                snprintf(c_dport, 6, "%d", dport);

                status = getaddrinfo(addr, c_dport, &hints, &name_to_ip);
                //cout << status << endl;

                for (struct addrinfo* p = name_to_ip; p!=NULL; p=p->ai_next){
                    void *addr;
                    char *ipver;
                    char ipstr[20];

                    if (p->ai_family == AF_INET){ // IPv4
                        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
                        addr = &(ipv4->sin_addr);
                        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
                        snprintf(dip, 20, "%s", ipstr);
                        break;
                        //ipver = "IPv4";
                    } 
                    else{ // IPV6
                        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
                        addr = &(ipv6->sin6_addr);
                        //ipver = "IPv6";
                    }

                    //printf(" %s: %s\n", ipver, ipstr);
                }
                
                //cout << addr << ": " << c_dport << endl;
                freeaddrinfo(name_to_ip);
            }

            //for (int i=0; i < 50; i++) cout << (int)buf[i] << ' ';
            //cout << endl;

            cout << "<S_IP>: " << sip << endl;
            cout << "<S_PORT>: " << sport << endl;
            cout << "<D_IP>: " << dip << endl;
            cout << "<D_PORT>: " << dport << endl;
            cout << "<Command>: "; cd==1 ? cout << "CONNECT" : cout << "BIND"; cout << endl;
           
            FILE *firewall = fopen("socks.conf", "r");
            char rule[200];
            bool permit = 0;
            while(fgets(rule, 200, firewall)){
                char check_ip[20], *ip_seg[4];
                snprintf(check_ip, 20, "%s", dip);
                ip_seg[0] = strtok(check_ip, ".");
                ip_seg[1] = strtok(NULL, ".");
                ip_seg[2] = strtok(NULL, ".");
                ip_seg[3] = strtok(NULL, ".");

                char *tok = strtok(rule, " ");
                char *mode = strtok(NULL, " ");
                int counter=0, i=0;


                for (tok = strtok(NULL, "."); tok != NULL; tok = strtok(NULL, ".\n")){
                    if (!strcmp(ip_seg[i++], tok) || !strcmp(tok, "*")){
                        counter++;
                    }
                    else {
                        break;
                    }
                }
                if (counter == 4){
                    if (cd == 1 && !strcmp(mode, "c")){
                        permit = 1;
                        break;
                    }
                    else if (cd == 2 && !strcmp(mode, "b")){
                        permit = 1;
                        break;
                    }
                }
            }
            fclose(firewall);

            if (!permit){
                unsigned char reply[8] = {0x00};

                cout << "<Reply>: Reject" << endl;
                reply[1] = 0x5B;
                write(slave_sock, reply, 8);
                exit(0);
            }

            if (cd == 1){
                int client_sockfd = 0;
                client_sockfd = socket(AF_INET , SOCK_STREAM , 0);

                if (client_sockfd == -1){
                    printf("Fail to create a socket.");
                }

                /*struct sockaddr_in info;
                bzero(&info,sizeof(info));
                info.sin_family = AF_INET;

                info.sin_addr.s_addr = inet_addr(dip);
                info.sin_port = htons(dport);

                int err = connect(client_sockfd,(struct sockaddr *)&info,sizeof(info));*/
                client_sockfd = connectDestHost(dip, dport);

                unsigned char reply[8] = {0x00};

                if (client_sockfd == -1){
                    cout << "<Reply>: Reject" << endl;
                    reply[1] = 0x5B;
                    write(slave_sock, reply, 8);
                    exit(0);
                }

                cout << "<Reply>: Accept" << endl;
                reply[1] = 0x5A;
                write(slave_sock, reply, 8);

                /*fd_set readfd;
                int maxfd = slave_sock > client_sockfd ? slave_sock : client_sockfd;
                FD_ZERO(&readfd);
                FD_SET(slave_sock, &readfd);
                FD_SET(client_sockfd, &readfd);

                while(1){
                    fd_set copy = readfd;
                    while(select(maxfd+1, &copy, NULL, NULL, NULL) < 1){
                        //cout << "Select Error!" << endl;
                        copy = readfd;
                    }
                    if (FD_ISSET(slave_sock, &copy)){
                        char data[2000];
                        int ret;
                        bzero(data, 2000);
                        ret = read(slave_sock, data, 2000);
                        //cout << buf << endl;
                        write(client_sockfd, data, ret);
                    }
                    if (FD_ISSET(client_sockfd, &copy)){
                        char data[2000];
                        int ret;
                        bzero(data, 2000);
                        ret = read(client_sockfd, data, 2000);
                        //cout << buf << endl;
                        write(slave_sock, data, ret);
                    }
                }*/
                redirectData(slave_sock, client_sockfd);

                close(client_sockfd);
            }

            else if (cd == 2){ // cd == 2
                int bindfd;
                struct sockaddr_in bind_addr, get_sock;
                if ( ( bindfd = socket(AF_INET, SOCK_STREAM, 0) ) == -1)
                    cout << "Bind mode: Socket create error" << endl;
                setsockopt(bindfd, SOL_SOCKET, SO_REUSEADDR, (const void *) &reuse_for_setsokopt, sizeof(int));
                setsockopt(bindfd, SOL_SOCKET, SO_REUSEPORT, (const void *) &reuse_for_setsokopt, sizeof(int));

	            bzero(&bind_addr, sizeof(bind_addr));
	            bzero(&get_sock, sizeof(get_sock));
                bind_addr.sin_family = AF_INET;
                bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                bind_addr.sin_port = htons(INADDR_ANY);

                while(bind(bindfd,(struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1){
                    counter_for_binding++;
                    if (counter_for_binding % 5000000 == 0) cout << '.' << flush;
                }

                unsigned int length;
                length = sizeof(get_sock);
                while(getsockname(bindfd, (struct sockaddr *) &get_sock, &length) == -1);

                while(listen(bindfd,30) == -1){
                    ;
                }

                unsigned char reply[8] = {0};
                unsigned int bind_port = ntohs(get_sock.sin_port);
                
                reply[1] = 90;
                reply[2] = bind_port / 256;
                reply[3] = bind_port % 256;
                write(slave_sock, reply, 8);

                int ftpfd;
                struct sockaddr_in ftp_addr;
                length = sizeof(ftp_addr);

                //cout << "Bind Port: " << (int)bind_port << endl;
                if (ftpfd = accept(bindfd, (struct sockaddr*) &ftp_addr, &length) == -1){
                    cout << "Bind mode: Accept error" << endl;
                }
                write(slave_sock, reply, 8);
                cout << "<Reply>: Accept" << endl;

                fd_set readfd;
                int maxfd = slave_sock > bindfd ? slave_sock : bindfd;
                maxfd = maxfd > ftpfd ? maxfd : ftpfd;
                FD_ZERO(&readfd);
                FD_SET(slave_sock, &readfd);
                FD_SET(ftpfd, &readfd);

                /*while(1){
                    char test[5000];
                    int ret;
                    ret = read(slave_sock, test, 5000);
                    cout << "client: "  <<ret << endl;
                    ret = read(ftpfd, test, 5000);
                    cout << ret << endl;
                }*/

                while(1){
                    fd_set copy = readfd;
                    while(select(maxfd+1, &copy, NULL, NULL, NULL) < 1){
                        //cout << "Select Error!" << endl;
                        copy = readfd;
                    }
                    if (FD_ISSET(slave_sock, &copy)){
                        char data[150000];
                        int ret;
                        bzero(data, 150000);
                        ret = read(slave_sock, data, 150000);
                        if (ret == 0){
                            cout << "ret == 0" << endl;
                            //write(ftpfd, 'EOF', 1);
                            continue;
                        }
                        //cout << "pid = " << getpid() << ", CLIENT can read" << endl;
                        //cout << data << endl;
                        write(ftpfd, data, ret);
                    }
                    if (FD_ISSET(ftpfd, &copy)){
                        cout << "pid = " << getpid() << ", FTP can read" << endl;
                        char data[150000];
                        int ret;
                        bzero(data, 150000);
                        ret = read(ftpfd, data, 150000);
                        //cout << data << endl;
                        write(slave_sock, data, ret);
                    }
                }

                //redirectData(slave_sock, ftpfd);

                close(bindfd);
                close(ftpfd);

            }

            close(slave_sock);
            exit(0);
        }

        else { // parent process
            close(slave_sock);
        }
	}

    close(sockfd);
    return 0;
}
