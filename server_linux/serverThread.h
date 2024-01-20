#pragma once

#include <thread>
#include <string>


using std::string;

class server;
class serverThread
{
public:
	serverThread(int, server*, int length = 3000);
	~serverThread();
	void run();
private:
	void getMessage(string);
	void sendMessage(string);
	void sendMessage(string, string);
	int login(string, string);
	string requestINFO(string, string);
	void query(string);

	string getUserData(string);
	bool checkPass(string, string);
	void setUser(string, string);

	char* buffer;
	int buflen;
	int thisSock;
	string curID;
	string userID;
	string userName;
	string retStr;
	server* thisServer;
	std::thread thisThread;//我必须把这个放最后
};

