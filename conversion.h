#pragma once

#include <string>
#include <sstream>

using namespace std;

template <typename T>
string convertstring(T v){
	stringstream ss;
	ss<<v;
	return ss.str();
}
