#include "parser.h"
#include "ast.h"
#include "expression.h"
#include "traceexception.h"
#include <iostream>
#include <string>

using namespace std;

int main(){
	string line;
	while(cin){
		getline(cin,line);
		if(line.size()==0)continue;
		ASTNode *node=nullptr;
		try {
			ASTNode *node=parseExpression(line);
			if(!saneTree(node)){
				cout<<"Insane expression!"<<endl;
				continue;
			}
			cerr<<*node<<endl; // cerr<<stringifyTree(node)<<endl;
			while(simplifyTree(node)){
				cerr<<*node<<endl; // cerr<<stringifyTree(node)<<endl;
			}
			cleanupTree(node);
			cout<<stringifyTree(node)<<endl;
		} catch(ParseError e){
			cout<<"\x1B[31m"<<e.what()<<"\x1B[0m"<<endl;
		}
		if(node)delete node;
	}
}
