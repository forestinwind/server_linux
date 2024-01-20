#include <signal.h>
#include "server.h"

int main()
{
    signal(SIGPIPE, SIG_IGN);
    server* ser = new server(40004);
    ser->begin();
    return 0;
}