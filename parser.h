#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cmath>

using namespace std;

enum asttype_t{
	AT_SUM, // A + B + C  (children: terms)
	AT_PRODUCT, // ABC  (children: factors)
	AT_VARIABLE, // x  (value: name)
	AT_NUMBER, // 42  (value: string parseable with strtold)
	AT_NEGATIVE, // -A  (child: negated factor)
	AT_RECIPROCAL, // 1/A  (child: factor divided by)
	AT_APPLY, // f(A...)  (value: function name, children: arguments)
	AT_PENDINGFUNCTION, // used in the parser  (value: function name)
};

unordered_map<asttype_t,string> asttypestring={
	{AT_SUM,"SUM"},
	{AT_PRODUCT,"PRODUCT"},
	{AT_VARIABLE,"VARIABLE"},
	{AT_NUMBER,"NUMBER"},
	{AT_NEGATIVE,"NEGATIVE"},
	{AT_RECIPROCAL,"RECIPROCAL"},
	{AT_APPLY,"APPLY"},
};

class ASTNode{
public:
	asttype_t type;
	vector<ASTNode*> children;
	string value;

	ASTNode(asttype_t type);
	ASTNode(asttype_t type,const vector<ASTNode*> &children);
	ASTNode(asttype_t type,const string &value);
	ASTNode(asttype_t type,const vector<ASTNode*> &children,const string &value);
	~ASTNode();
};

class ParseError : public exception{
	string whatstr;

public:
	ParseError(const string &s);
	const char* what() const noexcept;
};


#define NARGS(f,n) do {if(x.size()!=(n))throw ParseError(f " expects " #n "arguments");} while(0)
const unordered_map<string,function<long double(vector<long double>)>> functions={
	{"sin",   [](vector<long double> x){NARGS("sin",   1); return sin(x[0]);  }},
	{"cos",   [](vector<long double> x){NARGS("cos",   1); return cos(x[0]);  }},
	{"tan",   [](vector<long double> x){NARGS("tan",   1); return tan(x[0]);  }},
	{"asin",  [](vector<long double> x){NARGS("asin",  1); return asin(x[0]); }},
	{"acos",  [](vector<long double> x){NARGS("acos",  1); return acos(x[0]); }},
	{"atan",  [](vector<long double> x){NARGS("atan",  1); return atan(x[0]); }},
	{"arcsin",[](vector<long double> x){NARGS("arcsin",1); return asin(x[0]); }},
	{"arccos",[](vector<long double> x){NARGS("arccos",1); return acos(x[0]); }},
	{"arctan",[](vector<long double> x){NARGS("arctan",1); return atan(x[0]); }},
	{"sqrt",  [](vector<long double> x){NARGS("sqrt",  1); return sqrt(x[0]); }},
	{"exp",   [](vector<long double> x){NARGS("exp",   1); return exp(x[0]);  }},
	{"log",   [](vector<long double> x){NARGS("log",   1); return log(x[0]);  }},
	{"ln",    [](vector<long double> x){NARGS("ln",    1); return log(x[0]);  }},
	{"log10", [](vector<long double> x){NARGS("log10", 1); return log10(x[0]);}},
	{"log2",  [](vector<long double> x){NARGS("log2",  1); return log2(x[0]); }},

	{"pow",   [](vector<long double> x){NARGS("pow",   2); return pow(x[0],x[1]);     }},
	{"logb",  [](vector<long double> x){NARGS("logb",  2); return log(x[0])/log(x[1]);}},
};
#undef NARGS
const unordered_map<string,long double> constants={
	{"PI",M_PI},
	{"E",M_E},
	{"PHI",(1+sqrt(5))/2},
};


ASTNode* parseExpression(const string &expr);

