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


const char* sql_logout = "UPDATE userTable SET loged = false WHERE UID= &:1;;"; //用户登出，标记
const char* sql_login = "UPDATE userTable SET loged = true WHERE UID= &:1;;"; //用户登入
const char* sql_getName = "SELECT userName from userTable WHERE UID = &:1;"; //获取用户名字
const char* sql_getPassword = "SELECT password FROM userTable WHERE userName='&:1;';"; //获取密码
const char* sql_checkUser = "SELECT UID,loged FROM userTable WHERE userName='&:1;';"; //获取用户id和登陆情况(用于检查)
const char* sql_getUserInfo = "select * from userTable WHERE UID = &:1;;"; //获取用户资料
const char* sql_addUser = "insert into userTable(userName,password,loged) values('&:1;','&:2;',false);"; //添加用户
const char* sql_changeInfo = "UPDATE userTable SET &:1; = '&:2;' WHERE UID=&:3;;"; //修改用户资料

const char* sql_getFriend = "SELECT sendId FROM friendsTable WHERE acceptId = &:1;;"; 
const char* sql_checkFriend = "SELECT * from friendsTable where sendId = &:1; and acceptId = &:2;;";
const char* sql_addFriend = "insert into friendsTable(sendId, acceptId) values(&:1;, &:2;), (&:2;, &:1;);";
const char* sql_removeFriend = "delete from friendsTable where sendId = &:1; AND acceptId = &:2; OR sendId = &:2; AND acceptId = &:1;;";

const char* sql_getChatInfo = "SELECT sendTime,message,SID,RID FROM messageTable WHERE SID = &:1; AND RID = &:2; OR RID = &:1; AND SID = &:2; ORDER BY sendTime;";
const char* sql_addChatRec = "insert into messageTable(SID,RID,message,sendTime,loged) values(&:1;,&:2;,'&:3;','&:4;',false);";

const char* sql_getGroupInfo = "SELECT * from groupTable WHERE GID = &:1;;";

const char* sql_getGroup = "SELECT GID FROM groupMemberTable WHERE UID = &:1; AND level < 4;"; // 用户状态4表示未通过同意
const char* sql_getGroupMember = "SELECT level, UID FROM groupMemberTable WHERE GID = &:1; ORDER BY level;";
const char* sql_addGroup = "INSERT into groupMemberTable(GID,UID,level) values(&:1;, &:2;, 4);";
const char* sql_quitGroup = "delete from groupMemberTable where GID = &:1; AND UID = &:2;;";
const char* sql_setMemberLevel = "UPDATE groupMemberTable SET level = &:3; WHERE GID = &:1; AND UID = &:2;;";

const char* sql_getGroupChatInfo = "SELECT SID,sendTime,message FROM groupMessageTable WHERE GID = &:1; ORDER BY sendTime;";
const char* sql_addGroupChatInfo = "insert into groupMessageTable(GID,SID,message,sendTime,loged) values(&:1;,&:2;,'&:3;','&:4;',false);";

serverThread::serverThread(int socket, server* server, int length)
{
	buflen = length;
	std::cout << "begin\n";
	assert(buffer = (char*)malloc(buflen * sizeof(char)));
	std::cout << "try\n";
	thisSock = socket;
	thisServer = server;
	thisThread = new std::thread(&serverThread::run, this);
	thisThread->detach();
}
serverThread::~serverThread()
{
	std::cout << "finished\n";
	close(thisSock);
	free(buffer);
	if (userID != "")
	{
		thisServer->sqlComand(replaceStr(sql_logout, userID.data()));
		thisServer->userThread.erase(userID);
	}
}
void serverThread::run()
{
	while (true)
	{
		memset(buffer, 0, buflen * sizeof(char));
		int ret = recv(thisSock, buffer, buflen, 0);
		if (ret)
		{
			save += buffer;

			string temp = divide(save, END_CMD);
			while (temp != "")
			{
				std::cout << temp << "\n";
				getMessage(temp);
				temp = divide(save, END_CMD);
			}
		}
		else
		{
			std::cout << "quit\n";
			delete this;
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
		sendMessage("CHATRECORD", query(init));
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
	if (cmd == "ADDGROUP")
	{
		addGroup(init);
	}
	if (cmd == "REMOVE")
	{
		removeFriend(init);
		sendMessage("REMOVESUCCEES", removeFriend(init) + DIV_CMD);
	}
	if (cmd == "CHANGEINFO")
	{
		changeInfo(init);
	}
	if (cmd == "REQUESTGROUPINFO")
	{
		string SID = divide(init);
		string GID = divide(init);
		requestGroupINFO(SID, GID);
	}
	if (cmd == "QUERYGROUP")
	{
		sendMessage("GROUPCHATRECORD", groupQuery(init));
	}
	if (cmd == "SENDGROUPCHAT")
	{
		sendGroupChat(init);
	}
	if (cmd == "QUITGROUP")
	{
		quitGroup(init);
	}
	if (cmd == "GROUP_GETMEMBERINFO")
	{
		string GID = divide(init);
		sendMessage("GROUP_MEMBERINFO", GID + DIV_CMD + getUserData(init));
	}
	if (cmd == "GROUP_MEMEBRLEVELSET")
	{
		setGroupMemberLevel(init);
	}
}

void serverThread::changeInfo(string init)
{
	string type = divide(init);
	thisServer->sqlComand(replaceStr(sql_changeInfo, type.data(), init.data(), userID.data()));
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getFriend, userID.data()));

	MYSQL_ROW sqlRow;
	while (sqlRow = mysql_fetch_row(sqlRes))
	{
		if (thisServer->userThread[sqlRow[0]] != nullptr)
		{
			thisServer->userThread[sqlRow[0]]->freshFriendInfo(userID);
		}
	}
}
void serverThread::quitGroup(string init)
{
	string GID = divide(init);
	string SID = init;
	thisServer->sqlComand(replaceStr(sql_quitGroup, GID.data(), SID.data()));
	thisServer->forwardMess("QUITGROUP", SID, GID);
	this->forwardGroup(GID, "GROUP_MEMBERKICKED", GID + DIV_CMD + SID);
}
void serverThread::setGroupMemberLevel(string init)
{
	string GID = divide(init);
	string SID = divide(init);
	thisServer->sqlComand(replaceStr(sql_setMemberLevel, GID.data(), SID.data(), init.data()));
	if (init != "4")thisServer->forwardMess("JOINGROUP", SID, GID);
	this->forwardGroup(GID, "GROUP_MEMBERINFO", GID + DIV_CMD + init + DIV_CMD + getUserData(SID));
}
string serverThread::removeFriend(string init)
{
	string FID = divide(init);
	string SID = divide(init);
	thisServer->sqlComand(replaceStr(sql_removeFriend, FID.data(), SID.data()));
	return FID;
}
bool serverThread::addGroup(string init)
{
	thisServer->sqlComand(replaceStr(sql_addGroup, init.data(), userID.data()));
	string GID = init;
	this->forwardGroup(GID, "GROUP_MEMBERINFO", GID + DIV_CMD + "4" + DIV_CMD + getUserData(userID));
	std::cout << "joingroup " << userID << " " << GID;
	return true;
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
		setUser(sqlRow[0], userName);
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

	if (SID != FID)thisServer->forwardMess("CHATADD", FID, SID + DIV_CMD + time + DIV_CMD + init);
	thisServer->sqlComand(replaceStr(sql_addChatRec, SID.data(), FID.data(), init.data(), time.data()));

}
void serverThread::sendGroupChat(string init)
{
	string GID = divide(init);
	string SID = divide(init);
	string time = divide(init);
	string message = divide(init);
	thisServer->sqlComand(replaceStr(sql_addGroupChatInfo, GID.data(), SID.data(), message.data(), time.data()));

	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getGroupMember, GID.data()));
	MYSQL_ROW sqlRow;
	forwardGroup(GID, "GROUPCHATADD", GID + DIV_CMD + SID + DIV_CMD + time + DIV_CMD + message);
	while (sqlRow = mysql_fetch_row(sqlRes))
	{
		if (sqlRow[1] != SID)
		{
			thisServer->forwardMess("GROUPCHATADD", sqlRow[1], GID + DIV_CMD + SID + DIV_CMD + time + DIV_CMD + message);
		}
	}
}
string serverThread::groupQuery(string init)
{
	string SID = divide(init);
	string GID = divide(init);
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getGroupChatInfo, GID.data()));
	MYSQL_ROW sqlRow;
	retStr = GID + DIV_CMD;
	while (sqlRow = mysql_fetch_row(sqlRes))
	{
		retStr += sqlRow[0] + DIV_CMD + sqlRow[1] + DIV_CMD + sqlRow[2] + INF_CMD;
	}
	return retStr;
}
string serverThread::query(string init)
{
	string FID = divide(init);
	string SID = divide(init);

	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getName, SID.data()));
	
	MYSQL_ROW sqlRow;
	if (!(sqlRow = mysql_fetch_row(sqlRes)))return "";
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
	return retStr;
}
void serverThread::requestGroupINFO(string sid, string gid)
{
	bool vis = false;
	MYSQL_ROW sqlRow;
	string thisStr;
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getGroupInfo, gid.data()));
	if (sqlRow = mysql_fetch_row(sqlRes))
	{
		retStr = sqlRow[0] + DIV_CMD + sqlRow[1] + DIV_CMD;
		if (sqlRow[2]) retStr += sqlRow[2];
		retStr += DIV_CMD;
		if (sqlRow[3]) retStr += sqlRow[3];
		retStr += DIV_CMD;
		sendMessage("GROUPINFO", retStr);
	}
	else return;
	sqlRes = thisServer->sqlComand(replaceStr(sql_getGroupMember, gid.data()));
	while (sqlRow = mysql_fetch_row(sqlRes))
	{
		sendMessage("GROUP_MEMBERINFO", gid + DIV_CMD+ sqlRow[0] + DIV_CMD + getUserData(sqlRow[1]));
	}
	return;
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
				string temp = sqlRow[1];
				retStr += userID + DIV_CMD + userName + DIV_CMD + userID + INF_CMD;
				if (temp == "1") return 1;
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
			sqlRes = thisServer->sqlComand(replaceStr(sql_getGroup, userID.data()));
			while (sqlRow = mysql_fetch_row(sqlRes))
			{
				retStr += sqlRow[0] + DIV_CMD;
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
void serverThread::freshFriendInfo(string FID)
{
	sendMessage("USERINFO", requestINFO(userID, FID));
}
void serverThread::forwardGroup(string GID, string cmd, string init)
{
	MYSQL_RES* sqlRes = thisServer->sqlComand(replaceStr(sql_getGroupMember, GID.data()));
	MYSQL_ROW sqlRow;
	while (sqlRow = mysql_fetch_row(sqlRes))
	{
		std::cout << "forward" << sqlRow[1] << " " << init << "\n";
		thisServer->forwardMess(cmd, sqlRow[1], init);
	}
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
