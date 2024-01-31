#pragma once

#include <thread>
#include <string>


using std::string;

class server;
class serverThread
{
public:
	serverThread(int, server*, int length = 65536);
	~serverThread();
	void run();
	void sendMessage(string, string);
	void freshFriendInfo(string);
private:
	void getMessage(string);
	void sendMessage(string);
	int login(string, string);
	string requestINFO(string, string);
	string query(string);
	void sendChat(string);
	string regist(string);
	bool addFriend(string);
	bool addGroup(string);
	string removeFriend(string);
	void quitGroup(string);
	void changeInfo(string);
	void requestGroupINFO(string, string);
	string groupQuery(string);
	void sendGroupChat(string);
	void setGroupMemberLevel(string);

	void forwardGroup(string, string, string);


	string getUserData(string);
	bool checkPass(string, string);
	void setUser(string, string);

	bool running;
	int buflen;
	char* buffer;
	int thisSock;
	string curID;
	string userID;
	string userName;
	string retStr;
	string save;
	server* thisServer;
	std::thread* thisThread;//我必须把这个放最后
	//上面是假的，我必须把线程放在构造函数的最后面
};

