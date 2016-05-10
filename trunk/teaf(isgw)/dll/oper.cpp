// g++  oper.cpp  -fPIC -shared -o liboper.so -lACE  -I./ -I/usr/local/include/ace -I../../easyace/ -I../comm/ 
#include "oper.h"
#include "easyace_all.h"

int oper_init()
{
    return 0;
}

int oper_proc(char* req, char* ack, int& ack_len)
{
    ACE_DEBUG((LM_DEBUG, "oper start to proc\n"));
    
    snprintf(ack, ack_len, "info=%s", req);
    return 0;
}
