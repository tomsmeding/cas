#pragma once

#include "ast.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>

using namespace std;

class ParseError : public exception{
	string whatstr;

public:
	ParseError(const string &s);
	const char* what() const noexcept;
};


ASTNode* parseExpression(const string &expr);

