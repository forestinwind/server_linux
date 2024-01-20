#include <malloc.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <regex>
#include "serverThread.h"
#include "huurinn.h"


serverThread::serverThread(int socket, server* server, int length) :thisThread(&serverThread::run, this),buflen(length)
{
	assert(buffer = (char*)malloc(buflen * sizeof(char)));
	thisSock = socket;
	thisServer = server;
	thisThread.detach();
}
serverThread::~serverThread()
{
	if (userID != "")
	{
		string sqlComand = "UPDATE userTable SET loged = false WHERE UID=%1;";
		thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userID));
		thisServer->threadUser.erase(this);
	}
}
void serverThread::run()
{
	while (true)
	{
		memset(buffer, 0, buflen * sizeof(char));
		int ret = recv(thisSock, buffer, buflen, 0);
		if(ret)
		{
			string s;
			s.assign(buffer);
			send(thisSock, s.data(), s.size(), 0);
			while (s != "")
			{
				getMessage(divide(s, END_CMD));
			}
		}
		else
		{
			delete this;
		}
	}
}
void serverThread::getMessage(string init)
{
	int num = std::stoi(divide(init));
	string cmd = divide(init);
	if(cmd == "LOGIN")
	{
		login(divide(init), init);
	//	std::regex seleCMD("(.+)" + DIV_CMD + "(.+");
	}
}
void serverThread::login(string userName, string passWord)
{
	string sqlComand = "SELECT password FROM userTable WHERE userName='%1';";
	MYSQL_RES* sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userName));
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		if (checkPass(sqlRow[0], passWord))
		{
			sqlComand = "SELECT UID FROM userTable WHERE userName='%1';";
			sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userName));
			if (sqlRow = mysql_fetch_row(sqlRes)) 
			{
				setUser(sqlRow[0]);
			}
			else
			{
				sendMessage("ERROR");
				return;
			}
			sqlComand = "UPDATE userTable SET loged = true WHERE UID=%1;";
			thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userID));

		}
		else
		{
			sendMessage("WRONGPASS");
			return;
		}
	}
}
void serverThread::sendMessage(string init)
{
	send(thisSock, init.data(), init.size(), 0);
}
bool serverThread::checkPass(string sqlpass, string initpass)
{
	return sqlpass == initpass;
}
void serverThread::setUser(string ID)
{
	userID = ID;
	thisServer->threadUser[this] = ID;
}