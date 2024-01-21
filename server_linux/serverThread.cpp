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


const char* sql_logout = "UPDATE userTable SET loged = false WHERE UID= &:1;;";
const char* sql_login = "UPDATE userTable SET loged = true WHERE UID= &:1;;";
const char* sql_getName = "SELECT userName from userTable WHERE UID = &:1;";
const char* sql_getPassword = "SELECT password FROM userTable WHERE userName='&:1;';";
const char* sql_checkUser = "SELECT UID,loged FROM userTable WHERE userName='&:1;';";
const char* sql_getUserInfo = "select * from userTable WHERE UID = &:1;;";
const char* sql_addUser = "insert into userTable(userName,password,loged) values('&:1;','&:2;',false);";
const char* sql_changeInfo = "UPDATE userTable SET &:1; = '&:2;' WHERE UID=&:3;;";

const char* sql_getFriend = "SELECT sendId FROM friendsTable WHERE acceptId = &:1;;";
const char* sql_checkFriend = "SELECT * from friendsTable where sendId = &:1; and acceptId = &:2;;";
const char* sql_addFriend = "insert into friendsTable(sendId, acceptId) values(&:1;, &:2;), (&:2;, &:1;);";

const char* sql_getChatInfo = "SELECT sendTime,message,SID,RID FROM messageTable WHERE SID = &:1; AND RID = &:2; OR RID = &:1; AND SID = &:2; ORDER BY sendTime;";
const char* sql_addChatRec = "insert into messageTable(SID,RID,message,sendTime,loged) values(&:1;,&:2;,'&:3;','&:4;',false);";


serverThread::serverThread(int socket, server* server, int length)
{
	buflen = length;
	std::cout << "begin\n";
	assert(buffer = (char*)malloc(buflen * sizeof(char)));
	std::cout << "try\n";
	thisSock = socket;
	thisServer = server;
	thisThread = new std::thread(&serverThread::run, this);
	thisThread->join();
	std::cout << "finished\n";
	delete this;
}
serverThread::~serverThread()
{
	close(thisSock);
	free(buffer);
	if (userID != "")
	{
		thisServer->sqlComand(replaceStr(sql_logout, userID.data()));
		thisServer->userThread.erase(userID);
	}
}void serverThread::run()
{
	while (true)
	{
		memset(buffer, 0, buflen * sizeof(char));
		int ret = recv(thisSock, buffer, buflen, 0);
		if (ret)
		{
			save += buffer;

			std::cout << "savesize:" << save.size() << " ";
			std::cout << save << "\n";
			string temp = divide(save, END_CMD);
			while (temp != "")
			{
				std::cout << "tempsize:" << temp.size() << " ";
				getMessage(temp);
				temp = divide(save, END_CMD);
			}
		}
		else
		{
			std::cout << "quit\n";
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
	std::cout <<"\n:"<<init.length()<<" " << init << "\n";
	if (cmd == "LOGIN")
	{
		string userID = divide(init);
		string err = "0";
		int ret = login(userID, init);
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
	if (cmd == "SENDCHAT")
	{
		sendChat(init);
	}
	if (cmd == "REGISTER")
	{
		if (regist(init) == "")sendMessage("REGISTFAIL");
		else sendMessage("LOGINSUCCEES", retStr);
	}
	if (cmd == "ADDFRIEND")
	{
		if (addFriend(init))sendMessage("ADDFRIENDSUCCEES", retStr);
	}
	if (cmd == "CHANGEINFO")
	{
		changeInfo(init);
	}
}

void serverThread::changeInfo(string init)
{
	string type = divide(init);
	std::cout <<"initlen:" << init.size()<<"\n";
	thisServer->sqlComand(replaceStr(sql_changeInfo, type.data(), init.data(), userID.data()));
}
bool serverThread::addFriend(string init)
{
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(thisServer->sqlComand(replaceStr(sql_checkUser, init.data()))))
	{
		string FID = sqlRow[0];
		retStr = FID + DIV_CMD;
		thisServer->sqlComand(replaceStr(sql_addFriend, FID.data(), userID.data()));
		return true;
	}
	return false;
}
string serverThread::regist(string init)
{
	string userName = divide(init);
	string password = init;
	std::cout <<"regist:" << userName << " " << password << "\n";
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_checkUser, userName.data()));

	MYSQL_ROW sqlRow;
	if (mysql_fetch_row(sqlRes))return "";

	thisServer->sqlComand(replaceStr(sql_addUser, userName.data(), password.data()));
	sqlRes = thisServer->sqlComand(replaceStr(sql_checkUser, userName.data()));
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		retStr = sqlRow[0] + DIV_CMD + userName + DIV_CMD + sqlRow[0] + INF_CMD;
		std::cout << retStr << "\n";
		return sqlRow[0];
	}
	return "";
}
void serverThread::sendChat(string init)
{
	string SID = divide(init);
	string FID = divide(init);
	string time = divide(init);
	init = divide(init);

	thisServer->forwardMess(FID, SID + DIV_CMD + time + DIV_CMD + init);
	thisServer->sqlComand(replaceStr(sql_addChatRec, SID.data(), FID.data(), init.data(), time.data()));

}
void serverThread::query(string init)
{
	string FID = divide(init);
	string SID = divide(init);

	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getName, SID.data()));
	
	MYSQL_ROW sqlRow;
	if (!(sqlRow = mysql_fetch_row(sqlRes)))return;
	retStr = SID + DIV_CMD + sqlRow[0] + DIV_CMD;
	//sqlComand = "SELECT SID,RID,message,sendTime FROM messageTable WHERE SID = %1 AND RID = %2 OR RID=%1 AND SID = %2 ORDER BY sendTime;";
	sqlRes = thisServer->sqlComand(replaceStr(sql_getChatInfo, FID.data(), SID.data()));
	while(sqlRow = mysql_fetch_row(sqlRes))
	{
		string temp = sqlRow[0] + DIV_CMD + sqlRow[1];
		std::cout << temp << "\n";
		if (sqlRow[2] == FID)retStr += "0" + DIV_CMD + temp + INF_CMD;
		else retStr += "1" + DIV_CMD + temp + INF_CMD;
	}
}

string serverThread::requestINFO(string sid, string fid)
{
	if (sid == fid)
	{
		return getUserData(fid);
	}
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getFriend, sid.data(), fid.data()));
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		return getUserData(fid);
	}
	else return "";
}
int serverThread::login(string Name, string passWord)
{
	std::cout << "login\n";
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getPassword, Name.data()));
	std::cout << "let us select\n";
	MYSQL_ROW sqlRow;
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		std::cout << sqlRow[0] <<" " << "\n";
		if (checkPass(sqlRow[0], passWord))
		{
			sqlRes = thisServer->sqlComand(replaceStr(sql_checkUser, Name.data()));
			if (sqlRow = mysql_fetch_row(sqlRes))
			{
				setUser(sqlRow[0], Name);
				retStr += userID + DIV_CMD + userName + DIV_CMD + userID + INF_CMD;
				if (sqlRow[1] == "1") return 1;
			}
			else
			{
				return 4;//这应该不可能发生
			}
			thisServer->sqlComand(replaceStr(sql_login, userID.data()));

			sqlRes = thisServer->sqlComand(replaceStr(sql_getFriend, userID.data()));

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

	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getUserInfo, id.data()));
	MYSQL_ROW sqlRow;
	string ret;
	if (!(sqlRow = mysql_fetch_row(sqlRes))) return INF_CMD;
	ret += sqlRow[0] + DIV_CMD;
	ret += sqlRow[1] + DIV_CMD;
	if (sqlRow[3] == nullptr)ret += DIV_CMD;
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
	std::cout << "send" << init << "\n";
	send(thisSock, init.data(), init.size(), 0);
}
bool serverThread::checkPass(string sqlpass, string initpass)
{
	return sqlpass == initpass;
}
void serverThread::setUser(string ID, string name)
{
	userID = ID;
	userName = name;
	thisServer->userThread[ID] = this;
}