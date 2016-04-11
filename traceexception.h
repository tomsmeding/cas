#pragma once

#include <string>
#include <stdexcept>

using namespace std;

class TraceException : public exception{
	string tracestr;
	char buf[1024];

public:
	TraceException(const string &s);
	const char* what() const noexcept;
	const char* trace() const noexcept;
};
