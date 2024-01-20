#include <malloc.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>
#include <assert.h>
#include <regex>
#include "serverThread.h"
#include "server.h"
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
	free(buffer);
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
			break;
			//delete this;
		}
	}
}
void serverThread::getMessage(string init)
{
	curID = divide(init);
	string cmd = divide(init);
	retStr.clear();
	int ret;
	std::cout << cmd << "\n";
	if(cmd == "LOGIN")
	{
		string userID = divide(init);
		string err = "0";
		ret = login(userID, init);
		err[0] += ret;
		if (!ret)sendMessage("LOGINSUCCEES", retStr);
		else sendMessage("LOGINFAIL", err);
	//	std::regex seleCMD("(.+)" + DIV_CMD + "(.+");
	}
	if(cmd == "REQUESTINFO")
	{
		string SID = divide(init);
		string FID = divide(init);
		sendMessage("USERINFO", requestINFO(SID, FID));
	}
}
string serverThread::requestINFO(string sid, string fid)
{
	string sqlComand = "select * from friendsTable where sendId = %1 and acceptId = %2;";
	sqlComand = std::regex_replace(sqlComand, std::regex("(%1)"), sid);
	MYSQL_RES* sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%2)"), fid));
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		return getUserData(fid);
	}
	else return "";
}
int serverThread::login(string Name, string passWord)
{
	string sqlComand = "SELECT password FROM userTable WHERE userName='%1';";
	MYSQL_RES* sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), Name));
	std::cout << "let us select\n";
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		std::cout << sqlRow[0] <<" " << "\n";
		if (checkPass(sqlRow[0], passWord))
		{
			sqlComand = "SELECT UID,loged FROM userTable WHERE userName='%1';";
			sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), Name));
			if (sqlRow = mysql_fetch_row(sqlRes))
			{
				setUser(sqlRow[0], Name);
				std::cout << sqlRow[0] << " " << sqlRow[1] << "\n";
				retStr = userID + DIV_CMD + userName + DIV_CMD;
				if (sqlRow[1] == "1") return 1;
			}
			else
			{
				return 4;//这应该不可能发生
			}
			sqlComand = "UPDATE userTable SET loged = true WHERE UID=%1;";
			thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userID));

			sqlComand = "SELECT sendId FROM friendsTable WHERE acceptId = %1;";
			sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), userID));

			while (sqlRow = mysql_fetch_row(sqlRes))
			{
				retStr += sqlRow[0] + INF_CMD;
			}
		}
		else
		{
			return 2;
		}
	}
	else return 3;
	return 0;
}


string serverThread::getUserData(string id)
{
	string sqlComand = "select * from userTable WHERE UID = %1;";

	MYSQL_RES* sqlRes = thisServer->sqlComand(std::regex_replace(sqlComand, std::regex("(%1)"), id));
	MYSQL_ROW sqlRow;
	string ret;
	if (!(sqlRow = mysql_fetch_row(sqlRes))) return INF_CMD;
	std::cout << 1 << ":\n";
	ret += sqlRow[0] + DIV_CMD;
	ret += sqlRow[1] + DIV_CMD;
	if (sqlRow[3] == nullptr)ret +=DIV_CMD;
	else ret += sqlRow[3] + DIV_CMD;
	if (sqlRow[4] == nullptr)ret += DIV_CMD;
	else ret += sqlRow[4] + DIV_CMD;
	std::cout << ":" << ret << "\n";
	return ret + INF_CMD;
}
void serverThread::sendMessage(string init)
{
	init = curID + DIV_CMD + init + DIV_CMD + END_CMD;
	send(thisSock, init.data(), init.size(), 0);
}
void serverThread::sendMessage(string cmd, string init)
{
	init = curID + DIV_CMD + cmd + DIV_CMD + init + END_CMD;
	send(thisSock, init.data(), init.size(), 0);
}
bool serverThread::checkPass(string sqlpass, string initpass)
{
	std::cout << sqlpass << " " << initpass << ":" << sqlpass.size() << " " << initpass.size() << "\n";
	return sqlpass == initpass;
}
void serverThread::setUser(string ID, string name)
{
	userID = ID;
	userName = name;
	thisServer->threadUser[this] = ID;
}