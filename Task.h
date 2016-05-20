#pragma once
#include "Server.h"
#include <pthread.h>
#include <unistd.h>
#include <deque>
#include <iostream>
#include <vector>
#include <errno.h>
#include <string.h>

#include "Global.h"

using namespace std;

class Task
{
public:
    typedef void (Server::*callback_func_ptr)(int);
    void run();
    void setCallBack(callback_func_ptr);
    void setCallBackArg(int);
    void setServerInstance(Server*);
private:
  int callback_arg;
  callback_func_ptr cb_func;
  Server* instance;
};
