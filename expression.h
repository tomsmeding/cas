#pragma once

#include "ast.h"
#include <string>

using namespace std;

//returns false if the expression is ill-formed
bool saneTree(ASTNode *node);

//return whether anything changed
bool cleanupTree(ASTNode *node);
bool simplifyTree(ASTNode *node);

string stringifyTree(ASTNode *node);
