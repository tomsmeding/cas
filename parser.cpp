#include "parser.h"
#include "traceexception.h"
#include <iostream>
//#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <errno.h>
//#include <execinfo.h>

using namespace std;

/*inline string makebacktrace(){
	void *callstack[128];
	int nframes=backtrace(callstack,128);
	char **tracelist=backtrace_symbols(callstack,nframes);
	stringstream ss;
	for(int i=0;i<nframes;i++)ss<<tracelist[i]<<'\n';
	free(tracelist);
	return ss.str();
}*/

ostream& operator<<(ostream &os,const ASTNode &node);

ASTNode::ASTNode(asttype_t type)
	:type(type){
		//cout<<"Constructing ASTNode: "<<*this<<endl;
		//cout<<makebacktrace()<<endl;
	}
ASTNode::ASTNode(asttype_t type,const vector<ASTNode*> &children)
	:type(type),children(children){
		//cout<<"Constructing ASTNode: "<<*this<<endl;
		//cout<<makebacktrace()<<endl;
	}
ASTNode::ASTNode(asttype_t type,const string &value)
	:type(type),value(value){
		//cout<<"Constructing ASTNode: "<<*this<<endl;
		//cout<<makebacktrace()<<endl;
	}
ASTNode::ASTNode(asttype_t type,const vector<ASTNode*> &children,const string &value)
	:type(type),children(children),value(value){
		//cout<<"Constructing ASTNode: "<<*this<<endl;
		//cout<<makebacktrace()<<endl;
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


ParseError::ParseError(const string &s)
	:whatstr(s){}

const char* ParseError::what() const noexcept {
	return whatstr.data();
}


enum tokentype_t{
	TT_CHAR,
	TT_FUNCTION,
	TT_CONSTANT,
	TT_NUMBER,
	TT_SYMBOL,
	TT_NEGATIVE //value is "-"
};

unordered_map<tokentype_t,string> tokentypestring={
	{TT_CHAR,"CHAR"},
	{TT_FUNCTION,"FUNCTION"},
	{TT_CONSTANT,"CONSTANT"},
	{TT_NUMBER,"NUMBER"},
	{TT_SYMBOL,"SYMBOL"},
	{TT_NEGATIVE,"NEGATIVE"},
};

class Token{
public:
	tokentype_t type;
	string value;

	Token(tokentype_t type)
		:type(type){}
	Token(tokentype_t type,const string &value)
		:type(type),value(value){}
};

ostream& operator<<(ostream &os,const Token &token){
	return os<<'['<<tokentypestring.at(token.type)<<" \""<<token.value<<"\"]";
}


vector<Token> tokenise(const string &expr){
	int i;
	bool lastwasop=true;
	vector<Token> tokens;
	for(i=0;i<(int)expr.size();i++){
		if(isspace(expr[i]))continue;
		if(isdigit(expr[i])||expr[i]=='.'){
			if(!lastwasop){
				tokens.emplace_back(TT_SYMBOL,"*");
			}
			const char *startp=&expr[i];
			char *endp;
			errno=0;
			strtold(startp,&endp);
			if(endp==startp||errno!=0){
				throw ParseError("Cannot parse number starting at "+expr.substr(i,5));
			}
			tokens.emplace_back(TT_NUMBER,expr.substr(i,endp-startp));
			i+=endp-startp-1;
			lastwasop=false;
		} else if(strchr("+-*/%^(),",expr[i])!=nullptr){
			if(expr[i]=='-'&&lastwasop){
				tokens.emplace_back(TT_NEGATIVE,"-");
				lastwasop=true;
			} else {
				if(expr[i]=='('&&!lastwasop){
					tokens.emplace_back(TT_SYMBOL,"*");
				}
				tokens.emplace_back(TT_SYMBOL,string(1,expr[i]));
				lastwasop=expr[i]!=')';
			}
		} else if(isalpha(expr[i])){
			bool success=false;
			if(!lastwasop){
				tokens.emplace_back(TT_SYMBOL,"*");
			}
			for(const auto &fn : functions){
				const string &name=fn.first;
				if(expr.substr(i,name.size())==name){
					tokens.emplace_back(TT_FUNCTION,expr.substr(i,name.size()));
					i+=name.size()-1;
					success=true;
					lastwasop=true;
					break;
				}
			}
			if(!success)for(const auto &cn : constants){
				const string &name=cn.first;
				if(expr.substr(i,name.size())==name){
					tokens.emplace_back(TT_CONSTANT,expr.substr(i,name.size()));
					i+=name.size()-1;
					success=true;
					lastwasop=false;
					break;
				}
			}
			if(!success){
				tokens.emplace_back(TT_CHAR,string(1,expr[i]));
				lastwasop=false;
			}
		} else {
			throw ParseError("Unrecognised token starting at "+expr.substr(i,5));
		}
	}
	return tokens;
}


const unordered_map<string,int> precedence={
	{"(",-100},
	{"+",1},
	{"*",2},
	{"(-)",3},
	{"^",4},
};
const unordered_map<string,bool> rightassoc={
	{"(",true},
	{"+",false},
	{"*",false},
	{"(-)",true},
	{"^",true},
};
const unordered_map<string,int> arity={
	{"(-)",1},
	{"+",2},
	{"*",2},
	{"^",2},
};

void popOperator(vector<ASTNode*> &nodelist,vector<string> &opstack){
	if(opstack.size()==0)throw TraceException("Empty opstack in popOperator");
	string op=opstack.back();
	if(op=="[[function]]"){
		throw ParseError("Function without argument list");
	}
	opstack.pop_back();
	const int ar=arity.at(op);
	if((int)nodelist.size()<ar)throw ParseError("Not enough arguments to operator "+op);
	vector<ASTNode*> children(nodelist.begin()+(nodelist.size()-ar),nodelist.end());
	ASTNode *newnode;
	if(op=="^"){
		newnode=new ASTNode(AT_APPLY,children,"pow");
	} else if(op=="+"||op=="*"){
		newnode=new ASTNode(op=="+"?AT_SUM:AT_PRODUCT,children);
	} else if(op=="(-)"){
		newnode=new ASTNode(AT_NEGATIVE,children);
	} else throw logic_error("popOperator on operator "+op);
	nodelist.erase(nodelist.begin()+(nodelist.size()-ar),nodelist.end());
	nodelist.push_back(newnode);
}

void pushOperator(vector<ASTNode*> &nodelist,vector<string> &opstack,string op){
	if(op==")"){
		while(true){
			if(opstack.size()==0){
				throw ParseError("More closing parentheses than opening");
			}
			if(opstack.back()=="("){
				opstack.pop_back();
				if(opstack.size()&&opstack.back()=="[[function]]"){
					opstack.pop_back();
					int i;
					for(i=nodelist.size()-1;i>=0;i--){
						if(nodelist[i]->type==AT_PENDINGFUNCTION)break;
					}
					if(i==-1)throw ParseError("Function closing paren without pending");
					nodelist[i]->children.insert(nodelist[i]->children.end(),nodelist.begin()+(i+1),nodelist.end());
					nodelist[i]->type=AT_APPLY;
					nodelist.erase(nodelist.begin()+(i+1),nodelist.end());
				}
				break;
			}
			popOperator(nodelist,opstack);
		}
	} else if(op==","){
		while(true){
			if(opstack.size()==0){
				throw ParseError("Comma outside function argument list");
			}
			if(opstack.back()=="("){
				// Not popping the paren yet
				break;
			}
			popOperator(nodelist,opstack);
		}
	} else {
		int prec;
		try {
			prec=precedence.at(op);
		} catch(out_of_range){
			throw ParseError("Invalid operator "+op+" encountered");
		}
		while(opstack.size()){
			cerr<<__LINE__<<' '<<opstack.back()<<endl;
			const int otherprec=precedence.at(opstack.back());
			if(otherprec<prec||(otherprec==prec&&rightassoc.at(opstack.back()))){
				break;
			}
			popOperator(nodelist,opstack);
		}
		opstack.push_back(op);
	}
}

ASTNode* parse(const vector<Token> &tokens){
	vector<ASTNode*> nodelist;
	vector<string> opstack;
	int i;
	for(i=0;i<(int)tokens.size();i++){
		switch(tokens[i].type){
			case TT_CHAR:
				nodelist.push_back(new ASTNode(AT_VARIABLE,tokens[i].value));
				break;
			case TT_NUMBER:
				nodelist.push_back(new ASTNode(AT_NUMBER,tokens[i].value));
				break;
			case TT_SYMBOL:
				if(tokens[i].value=="("){
					opstack.push_back("(");
				} else if(tokens[i].value=="-"){
					pushOperator(nodelist,opstack,"+");
					pushOperator(nodelist,opstack,"(-)");
				} else {
					pushOperator(nodelist,opstack,tokens[i].value);
				}
				break;
			case TT_NEGATIVE:
				pushOperator(nodelist,opstack,"(-)");
				break;
			case TT_CONSTANT:
				nodelist.push_back(new ASTNode(AT_VARIABLE,tokens[i].value));
				break;
			case TT_FUNCTION:
				nodelist.push_back(new ASTNode(AT_PENDINGFUNCTION,tokens[i].value));
				opstack.push_back("[[function]]"); //the marker
				break;
		}
	}
	while(opstack.size()){
		popOperator(nodelist,opstack);
	}
	if(nodelist.size()!=1){
		throw ParseError("Multiple values in expression");
	}
	return nodelist[0];
}


ASTNode* parseExpression(const string &expr){
	return parse(tokenise(expr));
}


template <typename T>
ostream& operator<<(ostream &os,const vector<T> &v){
	if(v.size()==0)return os<<"[]";
	os<<'['<<v[0];
	for(size_t i=1;i<v.size();i++)os<<','<<v[i];
	return os<<']';
}

int main(){
	while(cin){
		string line;
		getline(cin,line);
		if(line.size()==0)continue;
		ASTNode *node=nullptr;
		try {
			vector<Token> tokens=tokenise(line);
			cout<<tokens<<endl;
			ASTNode *node=parse(tokens);
			cout<<*node<<endl;
		} catch(ParseError e){
			cout<<"\x1B[31m"<<e.what()<<"\x1B[0m"<<endl;
		}
		if(node)delete node;
	}
}
