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

const char* sql_logout = "UPDATE userTable SET loged = false WHERE UID= %1;";
const char* sql_login = "UPDATE userTable SET loged = true WHERE UID= %1;";
const char* sql_getName = "SELECT userName from userTable WHERE UID = %1";
const char* sql_getPassword = "SELECT password FROM userTable WHERE userName='%1';";
const char* sql_checkUser = "SELECT UID,loged FROM userTable WHERE userName='%1';";
const char* sql_getUserInfo = "select * from userTable WHERE UID = %1;";

const char* sql_getFriend = "SELECT sendId FROM friendsTable WHERE acceptId = %1;";
const char* sql_checkFriend = "SELECT * from friendsTable where sendId = %1 and acceptId = %2;";

const char* sql_getChatInfo = "SELECT sendTime,message,SID,RID FROM messageTable WHERE SID = %1 AND RID = %2 OR RID=%1 AND SID = %2 ORDER BY sendTime;";


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
		thisServer->sqlComand(replace(sql_logout, userID.data()));
		thisServer->threadUser.erase(this);
	}
}void serverThread::getMessage(string init)
{
	curID = divide(init);
	string cmd = divide(init);
	retStr.clear();
	int ret;
	std::cout << cmd << "\n";
	if (cmd == "LOGIN")
	{
		string userID = divide(init);
		string err = "0";
		ret = login(userID, init);
		err[0] += ret;
		if (!ret)sendMessage("LOGINSUCCEES", retStr);
		else sendMessage("LOGINFAIL", err);
		//	std::regex seleCMD("(.+)" + DIV_CMD + "(.+");
	}
	if (cmd == "REQUESTINFO")
	{
		string SID = divide(init);
		string FID = divide(init);
		sendMessage("USERINFO", requestINFO(SID, FID));
	}
	if (cmd == "QUERY")
	{
		query(init);
		sendMessage("CHATRECORD", retStr);
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
void serverThread::query(string init)
{
	string FID = divide(init);
	string SID = divide(init);

	MYSQL_RES* sqlRes = thisServer->sqlComand(replace(sql_getName, SID.data()));
	
	MYSQL_ROW sqlRow;
	if (!(sqlRow = mysql_fetch_row(sqlRes)))return;
	retStr = SID + DIV_CMD + sqlRow[0] + DIV_CMD;
	//sqlComand = "SELECT SID,RID,message,sendTime FROM messageTable WHERE SID = %1 AND RID = %2 OR RID=%1 AND SID = %2 ORDER BY sendTime;";
	sqlRes = thisServer->sqlComand(replace(sql_getChatInfo, FID.data(), SID.data()));
	while(sqlRow = mysql_fetch_row(sqlRes))
	{
		string temp = sqlRow[0] + DIV_CMD + sqlRow[1];
		if (sqlRow[2] == FID)retStr += "0" + DIV_CMD + temp + INF_CMD;
		else retStr += "1" + DIV_CMD + temp + INF_CMD;
	}
}

string serverThread::requestINFO(string sid, string fid)
{
	MYSQL_RES* sqlRes = thisServer->sqlComand(replace(sql_getFriend, sid.data(), fid.data()));
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		return getUserData(fid);
	}
	else return "";
}
int serverThread::login(string Name, string passWord)
{
	MYSQL_RES* sqlRes = thisServer->sqlComand(replace(sql_getPassword, Name.data()));
	std::cout << "let us select\n";
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		std::cout << sqlRow[0] <<" " << "\n";
		if (checkPass(sqlRow[0], passWord))
		{
			sqlRes = thisServer->sqlComand(replace(sql_checkUser, Name.data()));
			if (sqlRow = mysql_fetch_row(sqlRes))
			{
				setUser(sqlRow[0], Name);
				std::cout << sqlRow[0] << " " << sqlRow[1] << "\n";
				retStr += userID + DIV_CMD + userName + DIV_CMD + userID + INF_CMD;
				if (sqlRow[1] == "1") return 1;
			}
			else
			{
				return 4;//这应该不可能发生
			}
			thisServer->sqlComand(replace(sql_login, userID.data()));

			sqlRes = thisServer->sqlComand(replace(sql_getFriend, userID.data()));

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

	MYSQL_RES* sqlRes = thisServer->sqlComand(replace(sql_getUserInfo, id.data()));
	MYSQL_ROW sqlRow;
	string ret;
	if (!(sqlRow = mysql_fetch_row(sqlRes))) return INF_CMD;
	ret += sqlRow[0] + DIV_CMD;
	ret += sqlRow[1] + DIV_CMD;
	if (sqlRow[3] == nullptr)ret +=DIV_CMD;
	else ret += sqlRow[3] + DIV_CMD;
	if (sqlRow[4] == nullptr)ret += DIV_CMD;
	else ret += sqlRow[4] + DIV_CMD;
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