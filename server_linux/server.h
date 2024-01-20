#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <mysql.h>
#include <map>

#include "serverThread.h"

#pragma comment(lib,"libmysql.lib")



using std::string;

#define server_PORT 40004

class server
{
public:
    server(short unsigned int);
    void begin(int num = 5);
    MYSQL_RES* sqlComand(string);

    std::map<serverThread*, string> threadUser;
private:
    struct sockaddr_in serverIP;
    int sockID;
    MYSQL* thisMysql;
};