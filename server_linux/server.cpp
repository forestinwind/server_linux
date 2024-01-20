#include "server.h"
#include <functional>
#include <iostream>
#include <thread>
#include <cstdio>
server::server(short unsigned int port = server_PORT)
{
    thisMysql = new MYSQL;
    mysql_init(thisMysql);
    ///< 连接的数据库（句柄、主机名、用户名、密码、数据库名、端口号、socket指针、标记）
    assert(mysql_real_connect(thisMysql, "localhost", "root", "180819", "huurinn", 3306, nullptr, 0));
    serverIP.sin_family = AF_INET;
    serverIP.sin_port = htons(port);
    serverIP.sin_addr.s_addr = htonl(INADDR_ANY);
    sockID = socket(AF_INET, SOCK_STREAM, 0);
}
void server::begin(int num)
{
    assert(bind(sockID, (struct sockaddr*)&serverIP, sizeof(serverIP)) == 0);
    assert(listen(sockID, num) == 0);

    struct sockaddr_in clitAddr;
    socklen_t clitLen = sizeof(clitAddr);

    while (1) {
        int newSock = accept(sockID, (struct sockaddr*)&clitAddr, &clitLen);
        
        serverThread* newThread = new serverThread(newSock, this);

    //    std::thread mythread(std::mem_fn(&ChatServ::func), this, connfd);
    //    mythread.detach();
    }
}
MYSQL_RES* server::sqlComand(string cmd)
{
    std::cout << cmd << "\n";
    if (mysql_query(thisMysql, cmd.data()))std::cout << "succeed query\n";
    else std::cout << "fail";
    return mysql_store_result(thisMysql);
}