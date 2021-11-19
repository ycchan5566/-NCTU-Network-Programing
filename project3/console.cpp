#include <unistd.h>
#include <boost/asio.hpp>
#include <array>
#include <memory>
#include <utility>
#include <cstdio>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <iostream>
using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

io_service ioservice;


void output_shell(int session, std::string content){
    using boost::algorithm::replace_all;
    replace_all(content, "&", "&amp;");
    replace_all(content, "\"", "&quot;");
    replace_all(content, "\'", "&apos;");
    replace_all(content, "<", "&lt;");
    replace_all(content, ">", "&gt;");
    replace_all(content, "\n", "&NewLine;");
    replace_all(content, "\r", "");
    boost::format fmt("<script>document.getElementById('s%1%').innerHTML += '%2%';</script>");
    std::cout << fmt%session%content;
    std::cout.flush();
}

void output_command(int session, std::string content){
    using boost::algorithm::replace_all;
    replace_all(content, "&", "&amp;");
    replace_all(content, "\"", "&quot;");
    replace_all(content, "\'", "&apos;");
    replace_all(content, "<", "&lt;");
    replace_all(content, ">", "&gt;");
    replace_all(content, "\n", "&NewLine;");
    replace_all(content, "\r", "");
    boost::format fmt("<script>document.getElementById('s%1%').innerHTML += '<b>%2%</b>';</script>");
    std::cout << fmt%session%content;
    std::cout.flush();
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
                    if (!ec){
                        string output(bytes.begin(), bytes.begin() + length);
                        char a[4096];

                        output_shell(id, output);

                        if (output.find("% ") != string::npos) {
                            char line[15000];
                            if (fgets(line, 15000, file)){
                                string command(line);
                                output_command(id, command);
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

int main(int, char* const[], char* const envp[]) {
    char *request = getenv("QUERY_STRING");
    bool valid[5] = {0};
    int server_num = 0;
    vector<vector<string>> query;

    cout << "Content-type: text/html" << endl << endl;

    char html_template1[]="<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"UTF-8\" />\n<title>NP Project 3 Console</title>\n<link\nrel=\"stylesheet\"\nhref=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\nintegrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\ncrossorigin=\"anonymous\"\n/>\n<link\nhref=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\nrel=\"stylesheet\"\n/>\n<link\nrel=\"icon\"\ntype=\"image/png\"\nhref=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n/>\n<style>\n* {\nfont-family: 'Source Code Pro', monospace;\nfont-size: 1rem !important;\n}\nbody {\nbackground-color: #212529;\n}\npre {\ncolor: #cccccc;\n}\nb {\ncolor: #ffffff;\n}\n</style>\n</head>\n<body>\n<table class=\"table table-dark table-bordered\">\n<thead>\n<tr>\n";

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
    cout << frame << endl << endl;
    
    vector< shared_ptr<ShellSession> > client;
    for (int i=0; i<server_num; i++){
        string s("test_case/");
        s.append(query[i][2]);
        FILE *file;
        file = fopen(s.c_str(), "r");
        make_shared<ShellSession>(query[i][0], query[i][1], i, file)->start();
    }
    ioservice.run();

    return 0;
}
