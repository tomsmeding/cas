#include "traceexception.h"
#include "expression.h"
#include "parser.h"
#include "conversion.h"
#include <sstream>
#include <utility> //move
#include <errno.h>

using namespace std;

bool saneTree(ASTNode *node){
	if(node==nullptr)return false; //wat
	switch(node->type){
		case AT_SUM:
		case AT_PRODUCT:
			if(node->value.size()!=0)return false;
			if(node->children.size()==0)return false;
			for(size_t i=0;i<node->children.size();i++){
				if(!saneTree(node->children[i]))return false;
			}
			return true;

		case AT_VARIABLE:
			return node->children.size()==0&&node->value.size()!=0;

		case AT_NUMBER: {
			if(node->children.size()!=0||node->value.size()==0)return false;
			const char *startp=node->value.data();
			char *endp;
			errno=0;
			strtold(startp,&endp);
			if(errno!=0||endp-startp!=(long)node->value.size())return false;
			return true;
		}

		case AT_NEGATIVE:
		case AT_RECIPROCAL:
			return node->value.size()==0&&node->children.size()==1&&saneTree(node->children[0]);

		case AT_APPLY:
			if(node->value.size()==0)return false;
			for(ASTNode *child : node->children){
				if(!saneTree(child))return false;
			}
			return true;

		default:
			throw TraceException("Unexpected node type "+to_string(node->type)+" in "+__func__);
	}
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

namespace std {
	template <>
	struct less<ASTNode*>{
		bool operator()(const ASTNode *a,const ASTNode *b) const {
			return *a<*b;
		}
	};
}

void cleanupTree(ASTNode *node){
	switch(node->type){
		case AT_SUM:
		case AT_PRODUCT:
			if(node->children.size()>1){
				sort(node->children.begin(),node->children.end(),less<ASTNode*>());
			}
			break;

		case AT_VARIABLE:
		case AT_NUMBER:
			break;

		case AT_NEGATIVE:
		case AT_RECIPROCAL:
		case AT_APPLY:
			for(ASTNode *child : node->children){
				cleanupTree(child);
			}
			break;

		default:
			throw TraceException("Unexpected node type "+to_string(node->type)+" in "+__func__);
	}
}


bool foldNumbers(ASTNode *node){
	if(node->type!=AT_SUM&&node->type!=AT_PRODUCT)return false;
	int nfound=0;
	for(ASTNode *child : node->children){
		nfound+=child->type==AT_NUMBER;
	}
	if(nfound>1){
		long double number=node->type==AT_PRODUCT;
		for(size_t i=0;i<node->children.size();i++){
			ASTNode *child=node->children[i];
			if(child->type==AT_NUMBER){
				nfound++;
				double v=strtold(child->value.data(),nullptr);
				if(node->type==AT_SUM)number+=v;
				else number*=v;
				node->children.erase(node->children.begin()+i);
				delete child;
				i--;
			}
		}
		node->children.push_back(new ASTNode(AT_NUMBER,convertstring(number)));
		return true;
	}
	return false;
}

bool foldNegatives(ASTNode *node){
	if(node->type!=AT_PRODUCT)return false;
	int nfound=0;
	for(ASTNode *child : node->children){
		if(child->type==AT_NEGATIVE){
			nfound++;
			child->type=child->children[0]->type;
			child->value=child->children[0]->value;
			ASTNode *contents=child->children[0];
			child->children=move(child->children[0]->children);
			delete contents;
		}
	}
	if(nfound%2==1){
		ASTNode *prod=new ASTNode(AT_PRODUCT);
		prod->children=move(node->children);
		node->type=AT_NEGATIVE;
		node->children=vector<ASTNode*>(1,prod);
		return true;
	}
	return nfound>0;
}

//collapses recursive sums and products into one
//SUM(SUM(1,2),3) -> SUM(1,2,3), and similarly for product
bool collapseSums(ASTNode *node){
	bool changed=false;
	for(size_t i=0;i<node->children.size();i++){
		ASTNode *child=node->children[i];
		if(child->type==node->type){
			node->children.erase(node->children.begin()+i);
			node->children.insert(
				node->children.end(),
				child->children.begin(),
				child->children.end()
			);
			child->children.clear();
			delete child;
			changed=true;
			i--;
		}
	}
	return changed;
}

bool simplifyTree(ASTNode *node){
	bool changed=false;
	switch(node->type){
		case AT_SUM:
		case AT_PRODUCT:{
			for(ASTNode *child : node->children){
				changed=simplifyTree(child)||changed;
			}
			changed=collapseSums(node)||changed;
			changed=foldNumbers(node)||changed;
			if(foldNegatives(node)){
				simplifyTree(node);
				return true;
			}
			if(node->children.size()==1){
				ASTNode *child=node->children[0];
				node->type=child->type;
				node->value=move(child->value);
				node->children=move(child->children);
				delete child;
				changed=true;
				break;
			}
			break;
		}

		case AT_VARIABLE:
		case AT_NUMBER:
			break; //not much to do here

		case AT_NEGATIVE:
			changed=simplifyTree(node->children[0])||changed;
			if(node->children[0]->type==AT_NUMBER){
				node->type=AT_NUMBER;
				node->value=to_string(-strtold(node->children[0]->value.data(),nullptr));
				delete node->children[0];
				node->children.clear();
				changed=true;
			}
			break;

		case AT_RECIPROCAL:
			changed=simplifyTree(node->children[0])||changed;
			if(node->children[0]->type==AT_NUMBER){
				node->type=AT_NUMBER;
				node->value=to_string(1/strtold(node->children[0]->value.data(),nullptr));
				delete node->children[0];
				node->children.clear();
				changed=true;
			}
			break;

		case AT_APPLY: {
			bool allnumber=true;
			for(ASTNode *child : node->children){
				changed=simplifyTree(child)||changed;
				if(child->type!=AT_NUMBER)allnumber=false;
			}
			if(allnumber){
				vector<long double> values;
				values.reserve(node->children.size());
				for(ASTNode *child : node->children){
					values.push_back(strtold(child->value.data(),nullptr));
				}
				try {
					long double res=functions.at(node->value)(values);
					node->type=AT_NUMBER;
					for(ASTNode *child : node->children){
						delete child;
					}
					node->children.clear();
					node->value=convertstring(res);
					changed=true;
					break; //skips the tree functions below
				} catch(out_of_range){
					throw ParseError("Unknown function "+node->value);
				}
			}
			//TODO: handle tree functions here (derivative etc.)
			break;
		}

		default:
			throw TraceException("Unexpected node type "+to_string(node->type)+" in "+__func__);
	}
	return changed;
}


void stringifyTree(ASTNode *node,stringstream &ss){
	switch(node->type){
		case AT_SUM:
		case AT_PRODUCT:
			if(node->children.size()==0)break;
			ss<<'(';
			stringifyTree(node->children[0],ss);
			for(size_t i=1;i<node->children.size();i++){
				ss<<(node->type==AT_SUM?" + ":"*");
				stringifyTree(node->children[i],ss);
			}
			ss<<')';
			break;

		case AT_VARIABLE:
			ss<<node->value;
			break;

		case AT_NUMBER:
			ss<<node->value;
			break;

		case AT_NEGATIVE:
			ss<<'-';
			stringifyTree(node->children[0],ss);
			break;

		case AT_RECIPROCAL:
			ss<<"1/(";
			stringifyTree(node->children[0],ss);
			ss<<')';
			break;

		case AT_APPLY:
			ss<<node->value<<'(';
			if(node->children.size()==0){
				ss<<')';
				break;
			}
			stringifyTree(node->children[0],ss);
			for(size_t i=1;i<node->children.size();i++){
				ss<<',';
				stringifyTree(node->children[i],ss);
			}
			ss<<')';
			break;

		default:
			throw TraceException("Unexpected node type "+to_string(node->type)+" in "+__func__);
	}
}

string stringifyTree(ASTNode *node){
	stringstream ss;
	stringifyTree(node,ss);
	return ss.str();
}
