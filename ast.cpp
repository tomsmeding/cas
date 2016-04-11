#include "ast.h"

using namespace std;

unordered_map<asttype_t,string> asttypestring={
	{AT_SUM,"SUM"},
	{AT_PRODUCT,"PRODUCT"},
	{AT_VARIABLE,"VARIABLE"},
	{AT_NUMBER,"NUMBER"},
	{AT_NEGATIVE,"NEGATIVE"},
	{AT_RECIPROCAL,"RECIPROCAL"},
	{AT_APPLY,"APPLY"},
};


ASTNode::ASTNode(asttype_t type)
	:type(type){}

ASTNode::ASTNode(asttype_t type,const vector<ASTNode*> &children)
	:type(type),children(children){}

ASTNode::ASTNode(asttype_t type,const string &value)
	:type(type),value(value){}

ASTNode::ASTNode(asttype_t type,const vector<ASTNode*> &children,const string &value)
	:type(type),children(children),value(value){}

ASTNode::~ASTNode(){
	for(ASTNode *node : children)delete node;
}


ostream& operator<<(ostream &os,const ASTNode &node){
	static int depth=-1;
	const bool toplevel=depth==-1;
	if(toplevel)depth=0;
	os<<string(2*depth,' ')<<'('<<asttypestring.at(node.type)<<" \""<<node.value<<'"';
	if(node.children.size()==0)return os<<')';
	os<<'\n';
	depth++;
	for(ASTNode *n : node.children)os<<*n<<'\n';
	depth--;
	os<<string(2*depth,' ')<<')';
	if(toplevel)depth=-1;
	return os;
}
