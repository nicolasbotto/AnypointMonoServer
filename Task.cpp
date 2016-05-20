#include "Task.h"

void Task::run() {
    (instance->*cb_func)(callback_arg);
}

void Task::setServerInstance(Server* serverInstance)
{
    instance = serverInstance;
}

void Task::setCallBackArg(int arg)
{
    callback_arg = arg;
}

void Task::setCallBack(callback_func_ptr callback)
{
    cb_func = callback;
}
