#pragma once

#include "ast.h"
#include <string>

using namespace std;

//returns false if the expression is ill-formed
bool saneTree(ASTNode *node);

void cleanupTree(ASTNode *node);

//returns whether anything changed
bool simplifyTree(ASTNode *node);

string stringifyTree(ASTNode *node);


extern const unordered_map<string,function<long double(vector<long double>)>> functions;
extern const unordered_map<string,function<ASTNode*(vector<ASTNode*>)>> treefunctions;
extern const unordered_map<string,long double> constants;
