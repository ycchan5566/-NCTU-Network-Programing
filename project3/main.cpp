#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <sys/types.h>
//#include <sys/wait.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <unistd.h>
#define N_SERVERS 5

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;
using boost::asio::ip::tcp;

io_service ioservice;

char request[1024] = {0};
char cgi_name[50] = {0};
char req[300] = {0};
tcp::socket web_socket{ioservice};

string output_shell(int session, std::string content){
    using boost::algorithm::replace_all;
    replace_all(content, "&", "&amp;");
    replace_all(content, "\"", "&quot;");
    replace_all(content, "\'", "&apos;");
    replace_all(content, "<", "&lt;");
    replace_all(content, ">", "&gt;");
    replace_all(content, "\n", "&NewLine;");
    replace_all(content, "\r", "");
    boost::format fmt("<script>document.getElementById('s%1%').innerHTML += '%2%';</script>");
    return (fmt%session%content).str();
}

string output_command(int session, std::string content){
    using boost::algorithm::replace_all;
    replace_all(content, "&", "&amp;");
    replace_all(content, "\"", "&quot;");
    replace_all(content, "\'", "&apos;");
    replace_all(content, "<", "&lt;");
    replace_all(content, ">", "&gt;");
    replace_all(content, "\n", "&NewLine;");
    replace_all(content, "\r", "");
    boost::format fmt("<script>document.getElementById('s%1%').innerHTML += '<b>%2%</b>';</script>");
    return (fmt%session%content).str();
}

void parser(array<char, 1024> data){
    snprintf(request, 1024, "%s", data.data());
    char *tok = strtok(request, " \r\n");
    char *URI;
    char query[300] = {0};
    //setenv("REQUEST_METHOD", tok, 1);
    URI = strtok(NULL, " \r\n");
    //setenv("REQUEST_URI", URI, 1);
    tok = strtok(NULL, " \r\n");
    //setenv("SERVER_PROTOCOL", tok, 1);
    strtok(NULL, " \r\n");
    tok = strtok(NULL, " \r\n:");
    //setenv("HTTP_HOST", tok, 1);
    tok = strtok(NULL, " \r\n");
    //setenv("SERVER_PORT", tok, 1);
    snprintf(query, strlen(URI)+1, "%s", URI);
    //cout << query << endl;
    strtok(query, "?");
    tok = strtok(NULL, "");
    if (tok == NULL){
        //setenv("QUERY_STRING", "", 1);
    }
    else{
        //setenv("QUERY_STRING", tok, 1);
        snprintf(req, strlen(tok)+1, "%s", tok);
    }
    for (int i=1; query[i] != '\0'; i++){
        cgi_name[i-1] = query[i];
    }

}

class ShellSession : public std::enable_shared_from_this<ShellSession> {

private:
    int id;
    FILE *file;
    bool if_exit;
    tcp::resolver resolv{ioservice};
    tcp::socket tcp_socket{ioservice};
    char domain[100];
    char port[10];
    std::array<char, 4096> bytes;

public:
    ShellSession(string Domain, string Port, int ID, FILE *F){
        snprintf(domain, 100, "%s", Domain.c_str());
        snprintf(port, 10, "%s", Port.c_str());
        id = ID;
        file = F;
        if_exit = 0;
    }
    void start(){
        do_resolve();
    }

private:
    void do_read(){
        auto self(shared_from_this());
        tcp_socket.async_receive(
                buffer(bytes), 
                [this, self](boost::system::error_code ec, std::size_t length){
                cout << "id=" << id << ", fd = " << web_socket.native_handle() << endl;
                if (!ec){
                        string output(bytes.begin(), bytes.begin() + length);
                        char a[4096];

                        web_socket.write_some(buffer(output_shell(id, output)));
                        //async_write(web_socket, buffer(output_shell(id, output)), [this, self](boost::system::error_code ec, std::size_t length){;});
                         //cout << "id = " << id << endl;

                        if (output.find("% ") != string::npos) {
                            cout << "Send Command id = " << id << endl;
                            char line[15000];
                            if (fgets(line, 15000, file)){
                                string command(line);
                                web_socket.write_some(buffer(output_command(id, command)));
                                //async_write(web_socket, buffer(output_command(id, command)), [this, self](boost::system::error_code ec, std::size_t length){;});
                                tcp_socket.write_some(buffer(command));
                            }
                        }
                        do_read();
                    }
                });
    }

    void do_connect(tcp::resolver::iterator it){
        auto self(shared_from_this());
            tcp_socket.async_connect(
                    *it,
                    [this, self](const boost::system::error_code &ec){
                        if (!ec) do_read();
                    });
    }

    void do_resolve(){
        auto self(shared_from_this());
            tcp::resolver::query q{domain, port};
            resolv.async_resolve(
                    q, 
                    [this, self](const boost::system::error_code &ec,tcp::resolver::iterator it){
                        if (!ec) do_connect(it);
                    }
                    );
    }
};

void panel(tcp::socket socket){
    char context1[] = 
        "Content-type: text/html\r\n\r\n"
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
            "<title>NP Project 3 Panel</title>\n"
            "<link\n"
            "rel=\"stylesheet\"\n"
            "href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
            "integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
            "crossorigin=\"anonymous\"\n"
            "/>\n"
            "<link\n"
            "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
            "rel=\"stylesheet\"\n"
            "/>\n"
            "<link\n"
            "rel=\"icon\"\n"
            "type=\"image/png\"\n"
            "href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n"
            "/>\n"
            "<style>\n"
            "* {\n"
            "    font-family: \'Source Code Pro\', monospace;\n"
            "}\n"
            "</style>\n"
        "</head>\n"
        "<body class=\"bg-secondary pt-5\">"
        "<form action=\"console.cgi\" method=\"GET\">\n"
        "<table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n"
            "<thead class=\"thead-dark\">\n"
            "<tr>\n"
                "<th scope=\"col\">#</th>\n"
                "<th scope=\"col\">Host</th>\n"
                "<th scope=\"col\">Port</th>\n"
                "<th scope=\"col\">Input File</th>\n"
            "</tr>\n"
            "</thead>\n"
            "<tbody>";
    char context2[] = 
        "<tr>\n"
            "<th scope=\"row\" class=\"align-middle\">Session 1</th>\n"
            "<td>\n"
            "<div class=\"input-group\">\n"
                "<select name=\"h0\" class=\"custom-select\">\n"
                  "<option></option>"
                  "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                  "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                  "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                  "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                  "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                "</select>\n"
                "<div class=\"input-group-append\">\n"
                  "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
                "</div>\n"
              "</div>\n"
            "</td>\n"
            "<td>\n"
              "<input name=\"p0\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
            "</td>\n"
            "<td>\n"
              "<select name=\"f0\" class=\"custom-select\">\n"
                "<option></option>\n"
                "<option value=\"t1.txt\">t1.txt</option>"
                "<option value=\"t2.txt\">t2.txt</option>"
                "<option value=\"t3.txt\">t3.txt</option>"
                "<option value=\"t4.txt\">t4.txt</option>"
                "<option value=\"t5.txt\">t5.txt</option>"
                "<option value=\"t6.txt\">t6.txt</option>"
                "<option value=\"t7.txt\">t7.txt</option>"
                "<option value=\"t8.txt\">t8.txt</option>"
                "<option value=\"t9.txt\">t9.txt</option>"
                "<option value=\"t10.txt\">t10.txt</option>\n"
              "</select>\n"
            "</td>\n"
          "</tr>";
          char context3[] = 
        "<tr>\n"
            "<th scope=\"row\" class=\"align-middle\">Session 2</th>\n"
            "<td>\n"
            "<div class=\"input-group\">\n"
                "<select name=\"h1\" class=\"custom-select\">\n"
                  "<option></option>"
                  "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                  "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                  "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                  "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                  "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                "</select>\n"
                "<div class=\"input-group-append\">\n"
                  "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
                "</div>\n"
              "</div>\n"
            "</td>\n"
            "<td>\n"
              "<input name=\"p1\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
            "</td>\n"
            "<td>\n"
              "<select name=\"f1\" class=\"custom-select\">\n"
                "<option></option>\n"
                "<option value=\"t1.txt\">t1.txt</option>"
                "<option value=\"t2.txt\">t2.txt</option>"
                "<option value=\"t3.txt\">t3.txt</option>"
                "<option value=\"t4.txt\">t4.txt</option>"
                "<option value=\"t5.txt\">t5.txt</option>"
                "<option value=\"t6.txt\">t6.txt</option>"
                "<option value=\"t7.txt\">t7.txt</option>"
                "<option value=\"t8.txt\">t8.txt</option>"
                "<option value=\"t9.txt\">t9.txt</option>"
                "<option value=\"t10.txt\">t10.txt</option>\n"
              "</select>\n"
            "</td>\n"
          "</tr>";
          char context4[] = 
        "<tr>\n"
            "<th scope=\"row\" class=\"align-middle\">Session 3</th>\n"
            "<td>\n"
            "<div class=\"input-group\">\n"
                "<select name=\"h2\" class=\"custom-select\">\n"
                  "<option></option>"
                  "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                  "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                  "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                  "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                  "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                "</select>\n"
                "<div class=\"input-group-append\">\n"
                  "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
                "</div>\n"
              "</div>\n"
            "</td>\n"
            "<td>\n"
              "<input name=\"p2\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
            "</td>\n"
            "<td>\n"
              "<select name=\"f2\" class=\"custom-select\">\n"
                "<option></option>\n"
                "<option value=\"t1.txt\">t1.txt</option>"
                "<option value=\"t2.txt\">t2.txt</option>"
                "<option value=\"t3.txt\">t3.txt</option>"
                "<option value=\"t4.txt\">t4.txt</option>"
                "<option value=\"t5.txt\">t5.txt</option>"
                "<option value=\"t6.txt\">t6.txt</option>"
                "<option value=\"t7.txt\">t7.txt</option>"
                "<option value=\"t8.txt\">t8.txt</option>"
                "<option value=\"t9.txt\">t9.txt</option>"
                "<option value=\"t10.txt\">t10.txt</option>\n"
              "</select>\n"
            "</td>\n"
          "</tr>";
          char context5[] = 
        "<tr>\n"
            "<th scope=\"row\" class=\"align-middle\">Session 4</th>\n"
            "<td>\n"
            "<div class=\"input-group\">\n"
                "<select name=\"h3\" class=\"custom-select\">\n"
                  "<option></option>"
                  "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                  "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                  "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                  "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                  "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                "</select>\n"
                "<div class=\"input-group-append\">\n"
                  "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
                "</div>\n"
              "</div>\n"
            "</td>\n"
            "<td>\n"
              "<input name=\"p3\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
            "</td>\n"
            "<td>\n"
              "<select name=\"f3\" class=\"custom-select\">\n"
                "<option></option>\n"
                "<option value=\"t1.txt\">t1.txt</option>"
                "<option value=\"t2.txt\">t2.txt</option>"
                "<option value=\"t3.txt\">t3.txt</option>"
                "<option value=\"t4.txt\">t4.txt</option>"
                "<option value=\"t5.txt\">t5.txt</option>"
                "<option value=\"t6.txt\">t6.txt</option>"
                "<option value=\"t7.txt\">t7.txt</option>"
                "<option value=\"t8.txt\">t8.txt</option>"
                "<option value=\"t9.txt\">t9.txt</option>"
                "<option value=\"t10.txt\">t10.txt</option>\n"
              "</select>\n"
            "</td>\n"
          "</tr>";
          char context6[] = 
        "<tr>\n"
            "<th scope=\"row\" class=\"align-middle\">Session 5</th>\n"
            "<td>\n"
            "<div class=\"input-group\">\n"
                "<select name=\"h4\" class=\"custom-select\">\n"
                  "<option></option>"
                  "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>"
                  "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>"
                  "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>"
                  "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>"
                  "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                "</select>\n"
                "<div class=\"input-group-append\">\n"
                  "<span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n"
                "</div>\n"
              "</div>\n"
            "</td>\n"
            "<td>\n"
              "<input name=\"p4\" type=\"text\" class=\"form-control\" size=\"5\" />\n"
            "</td>\n"
            "<td>\n"
              "<select name=\"f4\" class=\"custom-select\">\n"
                "<option></option>\n"
                "<option value=\"t1.txt\">t1.txt</option>"
                "<option value=\"t2.txt\">t2.txt</option>"
                "<option value=\"t3.txt\">t3.txt</option>"
                "<option value=\"t4.txt\">t4.txt</option>"
                "<option value=\"t5.txt\">t5.txt</option>"
                "<option value=\"t6.txt\">t6.txt</option>"
                "<option value=\"t7.txt\">t7.txt</option>"
                "<option value=\"t8.txt\">t8.txt</option>"
                "<option value=\"t9.txt\">t9.txt</option>"
                "<option value=\"t10.txt\">t10.txt</option>\n"
              "</select>\n"
            "</td>\n"
          "</tr>";
        char context7[] =  
            "<tr>\n"
            "<td colspan=\"3\"></td>\n"
            "<td>\n"
              "<button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n"
            "</td>\n"
          "</tr>\n"
        "</tbody>\n"
      "</table>\n"
    "</form>\n"
  "</body>\n"
"</html>\n";
    string text1(context1), text2(context2), text3(context3), text4(context4), text5(context5), text6(context6), text7(context7);
    socket.write_some(buffer(text1+text2+text3+text4+text5+text6+text7));

}

void console(tcp::socket socket){
    char request[300];
    snprintf(request, 300, "%s", req);
    bool valid[5] = {0};
    int server_num = 0;
    vector<vector<string>> query;

    //cout << "Content-type: text/html" << endl << endl;

    char html_template1[]="Content-type: text/html\n\n<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\" />\n<title>NP Project 3 Console</title>\n<link\nrel=\"stylesheet\"\nhref=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\nintegrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\ncrossorigin=\"anonymous\"\n/>\n<link\nhref=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\nrel=\"stylesheet\"\n/>\n<link\nrel=\"icon\"\ntype=\"image/png\"\nhref=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n/>\n<style>\n* {\nfont-family: 'Source Code Pro', monospace;\nfont-size: 1rem !important;\n}\nbody {\nbackground-color: #212529;\n}\npre {\ncolor: #cccccc;\n}\nb {\ncolor: #ffffff;\n}\n</style>\n</head>\n<body>\n<table class=\"table table-dark table-bordered\">\n<thead>\n<tr>\n";

    char html_template2[]="</tr>\n</thead>\n<tbody>\n<tr>\n";

    char html_template3[]="</tr>\n</tbody>\n</table>\n</body>\n</html>";

    for (char *tok=strtok(request, " \n\r"); tok!=NULL; tok=strtok(NULL, " \n\r"));
    int count = 0;
    int count_valid = 0;
    for (int i=0; request[i] != '\0'; i++){
        if (request[i] == '='){
            count++;
            if (request[i+1] != '&'){
                count_valid++;
            }
        }
        if (count_valid == 3){
            count_valid = 0;
            valid[count/3-1] = 1;
        }
    }
    count = 0;

    for (char *tok=strtok(request, "=&"); tok!=NULL; ){
        vector<string> temp;
        if (valid[count]){
            tok = strtok(NULL, "=&");
            string s(tok);
            tok = strtok(NULL, "=&");
            tok = strtok(NULL, "=&");
            string t(tok);
            tok = strtok(NULL, "=&");
            tok = strtok(NULL, "=&");
            string r(tok);
            tok = strtok(NULL, "=&");
            temp.push_back(s);
            temp.push_back(t);
            temp.push_back(r);
            query.push_back(temp);
        }
        else{
            tok = strtok(NULL, "=&");
            tok = strtok(NULL, "=&");
            tok = strtok(NULL, "=&");
        }
        count++;
    }
    server_num = query.size();
    string content1, content2;
    for (int i=0; i<server_num; i++){
        char temp1[200], temp2[200];
        snprintf(temp1, 200, "<th scope=\"col\">%s:%s</th>\n", query[i][0].c_str(), query[i][1].c_str());
        content1.append(temp1);
        snprintf(temp2, 200, "<td><pre id=\"s%d\" class=\"mb-0\"></pre></td>\n", i);
        content2.append(temp2);
    }
    char frame[1500];
    snprintf(frame, 1500, "%s%s%s%s%s", html_template1, content1.c_str(), html_template2, content2.c_str(), html_template3);
    socket.write_some(buffer(frame));
    
    //vector< shared_ptr<ShellSession> > client;
    web_socket = move(socket);
    for (int i=0; i<server_num; i++){
        string s("test_case/");
        s.append(query[i][2]);
        FILE *file;
        file = fopen(s.c_str(), "r");
        printf("\nquery[0] = %s, query[1] = %s, i = %d, file = %s\n", query[i][0].c_str(), query[i][1].c_str(), i,  query[i][2].c_str());
        fflush(stdout);
        make_shared<ShellSession>(query[i][0], query[i][1], i, file)->start();
    }
}

class http_server {
    private:
        //boost::asio::io_service &_io_service;
        //boost::asio::signal_set _signal;
        tcp::acceptor _acceptor;
        tcp::socket _socket;
        array<char, 1024> _data;

    public:
        http_server(io_service &_io_service, uint16_t port) : //_io_service(io_service),
                                                                          //_signal(io_service, SIGCHLD),
                                                                          _acceptor(_io_service, {tcp::v4(), port}),
                                                                          _socket(_io_service) {
            //wait_signal();
            do_accept();
        }

 private:
  /*void wait_signal() {
    _signal.async_wait(
        [this](boost::system::error_code, int) {
          if (_acceptor.is_open()) {
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0);
            wait_signal();
          }
        });
  }*/

  void do_accept() {
    _acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            _socket = move(socket);
            for (int i=0; i<50; i++)
                cgi_name[i] = '\0';
            for (int i=0; i<300; i++)
                req[i] = '\0';


            do_read();

            /*_io_service.notify_fork(boost::asio::io_service::fork_prepare);

            if (fork()==0) {
              _io_service.notify_fork(boost::asio::io_service::fork_child);

              _acceptor.close();

              _signal.cancel();

              do_read();

            } else {
              _io_service.notify_fork(boost::asio::io_service::fork_parent);

              _socket.close();
              do_accept();
            }*/
          }
          do_accept();
        });
  }

  void do_read() {
      //cout << "Start" << endl;
      _socket.async_read_some(
                boost::asio::buffer(_data),
                [this](boost::system::error_code ec, size_t length) {
                  if (!ec) {
                      for (int i=0; i<length; i++) cout << _data[i];
                      cout << endl;
                    parser(_data);
                    //setenv("SERVER_ADDR", _socket.local_endpoint().address().to_string().c_str(), 1);
                    //setenv("REMOTE_ADDR", _socket.remote_endpoint().address().to_string().c_str(), 1);
                    //setenv("REMOTE_PORT", to_string(_socket.remote_endpoint().port()).c_str(), 1);
                    //for (int i=0; i<length; i++) cout << _data[i];
                    //cout << endl;

                    //dup2(_socket.native_handle(), 1);

                    char a[] = "HTTP/1.1 200 OK\r\n";
                    _socket.write_some(buffer(a));
                    //write(_socket.native_handle(), a, strlen(a));
                    //async_write(_socket, buffer(a), [this](boost::system::error_code, int){});

                    //cout << "Finish" << endl;
                    /*char *argv[] = {nullptr};
                    if (execv(cgi_name, argv)==-1) {
                        cout << "Exec Error" << endl;
                        exit(-1);
                    }*/
                    //do_read();
                    //_socket.close();
                    //_acceptor.close();
                    cout << cgi_name << endl;
                    cout << strcmp(cgi_name, "panel.cgi") << endl;
                    cout << strcmp(cgi_name, "console.cgi") << endl;
                    if (!strcmp(cgi_name, "panel.cgi")){
                        panel(move(_socket));
                    } 
                    else if (!strcmp(cgi_name, "console.cgi")){
                        console(move(_socket));
                    }
                    
                    do_accept();
                  }
                });
    }

};

int main(int argc, char *argv[]) {
    //signal(SIGCHLD, SIG_IGN);
    try{
        if (argc!=2) {
        cerr << "Usage: http_server [port]\n";
        return 1;
    }
    http_server server(ioservice, atoi(argv[1]));
    ioservice.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
