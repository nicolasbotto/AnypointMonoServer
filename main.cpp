#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "Server.h"
#include <pthread.h>

typedef unsigned char byte;

//#define BUFFER_SIZE 4096
#define SOCKET_NAME "/tmp/dotnetsocket"

using namespace std;


//put in server
static bool isConnected = false;
/*
void* HandleConnection(void*)
{
    cout << "running new thread" << endl;
    server.Start();
    isConnected = true;
    
    
    cout << "finished waiting" << endl;
    server.Stop();
}
*/
int main(int argc, char**argv) {
    printf("%s\n", "Starting server");
    
    string path = string(argv[1]);
    //"/home/test/Documents/DevKit"
    
    printf("%i", getpid());
    
    Server server(SOCKET_NAME, path.c_str());
    
    
    server.Start();
    isConnected = true;
    
    /*
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, "/tmp/test");
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }
    
    cout << "enter message" << endl;
    char input[BUFFER_SIZE] = {};
    cin.getline(input, BUFFER_SIZE);
    
   // cout << input << endl;
    
    while(strcmp(input, "end") != 0)
    {
        cout << "enter id" << endl;
        char idIn[BUFFER_SIZE] = {};
        cin >> idIn; 
        int id = atoi(idIn);
        DTO buffer = DTO(id,input);
        
        //if (send(s, input, strlen(input), 0) == -1) {
        if (send(s, (void*)&buffer, sizeof(DTO), 0) == -1) {
            perror("send");
            exit(1);
        }

        //if ((t=recv(s, str, 100, 0)) > 0) {
        //    str[t] = '\0';
        //    printf("echo> %s", str);
        //} else {
        //    if (t < 0) perror("recv");
        //    else printf("Server closed connection\n");
        //    exit(1);
       // }
        
        cout << "enter message" << endl;
        
        cin.getline(input, BUFFER_SIZE);
    
        //cout << input << endl;
    }
    
    close(s);
    exit(0);
   // Server server("/tmp/test");
    
   // server.Start();
    
   // server.Stop();
    */
}

