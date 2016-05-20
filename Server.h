#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include "JniManager.h"

#define BUFFER_SIZE 256

typedef unsigned char byte;
using namespace std;

class Server 
{
    public:
        Server(const char*, const char*);
        void Start();
        void Stop();
        void HandleRequest(int);

    private:
        void InitMono(const char*);
        const char* socketName;
        struct sockaddr_un name;
        int connection_socket;
        bool started;
};
