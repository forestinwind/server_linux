#include <iostream>
#include <regex>
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
string replaceStr(string init, ...)
{
	int cnt = 0;
	while(++cnt)
		if (!std::regex_search(init, std::regex("&:" + std::to_string(cnt) + ";")))break;
	va_list thisList;
	va_start(thisList, init);
	for (int i = 1; i < cnt; i++)
	{
		string now = va_arg(thisList, char*);
		init = std::regex_replace(init, std::regex("&:" + std::to_string(i) + ";"), now);
	}
	va_end(thisList);
	return init;

}