#include <iostream>
#include "huurinn.h"

string divide(string& init, string str)
{
	int loc = init.find(str);
	string ret;
	if(loc == string::npos)
	{
		string ret = init;
		init = "";
		return ret;
	}
	ret.assign(init, 0, loc);
	init = init.substr(loc + str.size());
	return ret;
}