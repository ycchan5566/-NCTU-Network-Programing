#include <unistd.h>
#include <cstring>
#include <iostream>
using namespace std;

int main(int, char* const[], char* const envp[]) {
  /* [Required] HTTP Header */
  cout << "Content-type: text/html" << endl << endl;

  char cwd[4096];
  getcwd(cwd, sizeof(cwd));
  cout << "<b>Current Working Directory</b>" << endl;
  cout << "<pre>" << cwd << "</pre>" << endl;

  cout << "<b>Environment Variables</b>" << endl;
  cout << "<pre>" << endl;
  //for (int i = 0; envp[i]; ++i){
      cout << "REQUEST_METHOD = " << getenv("REQUEST_METHOD") << endl;
      cout << "REQUEST_URI = " << getenv("REQUEST_URI") << endl;
      cout << "QUERY_STRING = "<< getenv("QUERY_STRING") << endl; 
      cout <<  "SERVER_PROTOCOL = "<< getenv("SERVER_PROTOCOL")<< endl;
      cout <<  "HTTP_HOST = "<< getenv("HTTP_HOST")<< endl;
      cout <<  "SERVER_ADDR = "<< getenv("SERVER_ADDR")<< endl;
      cout <<  "SERVER_PORT = "<< getenv("SERVER_PORT")<< endl;
      cout <<  "REMOTE_ADDR = "<< getenv("REMOTE_ADDR")<< endl;
      cout <<  "REMOTE_PORT = "<< getenv("REMOTE_PORT")<< endl;
    //cout << envp[i] << endl;
  //}
  cout << "</pre>" << endl;
  return 0;
}
