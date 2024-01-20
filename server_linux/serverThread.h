#pragma once

#include <thread>

#include "server.h"


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
	void login(string, string);


	bool checkPass(string, string);
	void setUser(string);

	char* buffer;
	int buflen;
	int thisSock;
	string userID;
	server* thisServer;
	std::thread thisThread;//我必须把这个放最后
};

