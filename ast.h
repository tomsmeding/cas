#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

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

extern unordered_map<asttype_t,string> asttypestring;

class ASTNode{
public:
	asttype_t type;
	vector<ASTNode*> children;
	string value;

	ASTNode(asttype_t type);
	ASTNode(asttype_t type,const vector<ASTNode*> &children);
	ASTNode(asttype_t type,const string &value);
	ASTNode(asttype_t type,const vector<ASTNode*> &children,const string &value);
	explicit ASTNode(const ASTNode &other);
	~ASTNode();
};

ostream& operator<<(ostream &os,const ASTNode &node);

bool operator==(const ASTNode &a,const ASTNode &b);
bool operator!=(const ASTNode &a,const ASTNode &b);

bool operator<(const ASTNode &a,const ASTNode &b);

namespace std {
	template <>
	struct less<ASTNode*>{
		bool operator()(const ASTNode *a,const ASTNode *b) const;
	};
}
