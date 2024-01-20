#pragma once

#include <string>

#define END_CMD (string)"&end;"
#define DIV_CMD (string)"&div;"
#define INF_CMD (string)"&inf;"

using std::string;
class huurinn
{
};

string divide(string&, string str = DIV_CMD);
