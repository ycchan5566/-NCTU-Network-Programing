#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using boost::asio::ip::tcp;
using namespace std;

boost::asio::io_service io_service;

char request[1024] = {0};
char cgi_name[50] = {0};

void parser(array<char, 1024> data){
    snprintf(request, 1024, "%s", data.data());
    char *tok = strtok(request, " \r\n");
    char *URI;
    char query[300] = {0};
    setenv("REQUEST_METHOD", tok, 1);
    URI = strtok(NULL, " \r\n");
    setenv("REQUEST_URI", URI, 1);
    tok = strtok(NULL, " \r\n");
    setenv("SERVER_PROTOCOL", tok, 1);
    strtok(NULL, " \r\n");
    tok = strtok(NULL, " \r\n:");
    setenv("HTTP_HOST", tok, 1);
    tok = strtok(NULL, " \r\n");
    setenv("SERVER_PORT", tok, 1);
    snprintf(query, strlen(URI)+1, "%s", URI);
    //cout << query << endl;
    strtok(query, "?");
    tok = strtok(NULL, "");
    if (tok == NULL){
        setenv("QUERY_STRING", "", 1);
    }
    else{
        setenv("QUERY_STRING", tok, 1);
    }
    for (int i=1; query[i] != '\0'; i++){
        cgi_name[i-1] = query[i];
    }

}

class http_server {
    private:
        boost::asio::io_service &_io_service;
        boost::asio::signal_set _signal;
        tcp::acceptor _acceptor;
        tcp::socket _socket;
        array<char, 1024> _data;

    public:
        http_server(boost::asio::io_service &io_service, uint16_t port) : _io_service(io_service),
                                                                          _signal(io_service, SIGCHLD),
                                                                          _acceptor(io_service, {tcp::v4(), port}),
                                                                          _socket(io_service) {
            wait_signal();
            do_accept();
        }

 private:
  void wait_signal() {
    _signal.async_wait(
        [this](boost::system::error_code, int) {
          if (_acceptor.is_open()) {
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0);
            wait_signal();
          }
        });
  }

  void do_accept() {
    _acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            _socket = move(socket);

            _io_service.notify_fork(boost::asio::io_service::fork_prepare);

            if (fork()==0) {
              _io_service.notify_fork(boost::asio::io_service::fork_child);

              _acceptor.close();

              _signal.cancel();

              do_read();

            } else {
              _io_service.notify_fork(boost::asio::io_service::fork_parent);

              _socket.close();
              do_accept();
            }
          } else {
            cerr << "Accept error" << endl;
            do_accept();
          }
        });
  }

  void do_read() {
      _socket.async_read_some(
                boost::asio::buffer(_data),
                [this](boost::system::error_code ec, size_t length) {
                  if (!ec) {
                    parser(_data);
                    setenv("SERVER_ADDR", _socket.local_endpoint().address().to_string().c_str(), 1);
                    setenv("REMOTE_ADDR", _socket.remote_endpoint().address().to_string().c_str(), 1);
                    setenv("REMOTE_PORT", to_string(_socket.remote_endpoint().port()).c_str(), 1);
                    //for (int i=0; i<length; i++) cout << _data[i];
                    //cout << endl;

                    dup2(_socket.native_handle(), 1);

                    cout << "HTTP/1.1 200 OK\r\n";
                    cout.flush();

                    char *argv[] = {nullptr};
                    if (execv(cgi_name, argv)==-1) {
                        cout << "Exec Error" << endl;
                        exit(-1);
                    }
                  }
                });
    }

};

int main(int argc, char *argv[]) {
    signal(SIGCHLD, SIG_IGN);
    if (argc!=2) {
        cerr << "Usage: http_server [port]\n";
        return 1;
    }
    http_server server(io_service, atoi(argv[1]));
    io_service.run();
}
