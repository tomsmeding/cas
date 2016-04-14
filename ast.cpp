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

ASTNode::ASTNode(const ASTNode &other)
	:type(other.type),value(other.value){
	children.reserve(other.children.size());
	for(ASTNode *n : other.children){
		children.push_back(new ASTNode(*n));
	}
}

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

bool operator==(const ASTNode &a,const ASTNode &b){
	if(a.type!=b.type)return false;
	if(a.value!=b.value)return false;
	if(a.children.size()!=b.children.size())return false;
	size_t i,j;
	//a \subseteq b
	for(i=0;i<a.children.size();i++){
		for(j=0;j<b.children.size();j++){
			if(*a.children[i]==*b.children[i])break;
		}
		if(j==b.children.size())return false;
	}
	//b \subseteq a
	for(i=0;i<b.children.size();i++){
		for(j=0;j<a.children.size();j++){
			if(*a.children[i]==*b.children[i])break;
		}
		if(j==b.children.size())return false;
	}
	return true;
}
bool operator!=(const ASTNode &a,const ASTNode &b){
	return !operator==(a,b);
}

const int asttypeorder[]={
	6, //SUM
	7, //PRODUCT
	3, //VARIABLE
	2, //NUMBER
	1, //NEGATIVE
	4, //RECIPROCAL
	5, //APPLY
};

bool operator<(const ASTNode &a,const ASTNode &b){
	if(a.type!=b.type){
		return asttypeorder[a.type]<asttypeorder[b.type];
	}
	switch(a.type){
		case AT_SUM:
		case AT_PRODUCT:
			return a.children.size()<b.children.size();

		case AT_VARIABLE:
			if(a.value.size()!=b.value.size()){
				return a.value.size()<b.value.size();
			}
			return a.value<b.value;

		case AT_NUMBER:
			return strtold(a.value.data(),nullptr)<strtold(b.value.data(),nullptr);

		case AT_NEGATIVE:
		case AT_RECIPROCAL:
			return *a.children[0]<*b.children[0];

		case AT_APPLY:
			return a.value<b.value; //or something

		default:
			return false; //equal?
	}
}

namespace std {
	bool less<ASTNode*>::operator()(const ASTNode *a,const ASTNode *b) const {
		return *a<*b;
	}
}
