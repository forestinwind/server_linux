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
private:
	void getMessage(string);
	void sendMessage(string);
	int login(string, string);
	string requestINFO(string, string);
	void query(string);
	void sendChat(string);
	string regist(string);
	bool addFriend(string);
	void changeInfo(string);

	string getUserData(string);
	bool checkPass(string, string);
	void setUser(string, string);

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

