#include "traceexception.h"
#include "expression.h"
#include "parser.h"
#include "conversion.h"
#include <sstream>
#include <set>
#include <utility> //move
#include <errno.h>

using namespace std;

//returns new tree, not yet simplified
ASTNode* derivative(ASTNode *node,string var){
	ASTNode *res;
	switch(node->type){
		case AT_SUM:
			res=new ASTNode(AT_SUM);
			res->children.reserve(node->children.size());
			for(ASTNode *child : node->children){
				res->children.push_back(derivative(child,var));
			}
			return res;

		case AT_PRODUCT:
			res=new ASTNode(AT_SUM);
			res->children.reserve(node->children.size());
			for(size_t i=0;i<node->children.size();i++){
				ASTNode *prod=new ASTNode(AT_PRODUCT);
				prod->children.reserve(node->children.size());
				for(size_t j=0;j<node->children.size();j++){
					if(j==i)prod->children.push_back(derivative(node->children[j],var));
					else prod->children.push_back(new ASTNode(*node->children[j]));
				}
				res->children.push_back(prod);
			}
			return res;

		case AT_VARIABLE:
			return new ASTNode(AT_NUMBER,node->value==var?"1":"0");

		case AT_NUMBER:
			return new ASTNode(AT_NUMBER,"0");

		case AT_NEGATIVE:
			return new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,derivative(node->children[0],var)));

		case AT_RECIPROCAL:{
			ASTNode *numerator=derivative(node->children[0],var);
			ASTNode *denominator=new ASTNode(
				AT_APPLY,
				"pow",
				vector<ASTNode*>{new ASTNode(*node->children[0]),new ASTNode(AT_NUMBER,"2")}
			);
			ASTNode *prod=new ASTNode(
				AT_PRODUCT,
				vector<ASTNode*>{numerator,new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,denominator))}
			);
			return new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,prod));
		}

		case AT_APPLY:{
			ASTNode *a1=node->children.size()<1?nullptr:node->children[0];
			ASTNode *a2=node->children.size()<2?nullptr:node->children[1];
			if(node->value=="sin"){
				return new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(AT_APPLY,"cos",vector<ASTNode*>(1,new ASTNode(*a1))),
					derivative(a1,var)
				});
			} else if(node->value=="cos"){
				return new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,
					new ASTNode(AT_PRODUCT,vector<ASTNode*>{
						new ASTNode(AT_APPLY,"sin",vector<ASTNode*>(1,new ASTNode(*a1))),
						derivative(a1,var)
					})
				));
			} else if(node->value=="tan"){
				return new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,
						new ASTNode(AT_APPLY,"pow",vector<ASTNode*>{
							new ASTNode(AT_APPLY,"cos",vector<ASTNode*>(1,new ASTNode(*a1))),
							new ASTNode(AT_NUMBER,"2")
						})
					)),
					derivative(a1,var)
				});
			} else if(node->value=="asin"||node->value=="arcsin"||
				      node->value=="acos"||node->value=="arccos"){
				res=new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,
						new ASTNode(AT_APPLY,"sqrt",vector<ASTNode*>(1,
							new ASTNode(AT_SUM,vector<ASTNode*>{
								new ASTNode(AT_NUMBER,"1"),
								new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,
									new ASTNode(AT_APPLY,"pow",vector<ASTNode*>{
										new ASTNode(*a1),
										new ASTNode(AT_NUMBER,"2")
									})
								))
							})
						))
					)),
					derivative(a1,var)
				});
				if(node->value=="asin"||node->value=="arcsin")return res;
				else return new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,res));
			} else if(node->value=="atan"||node->value=="arctan"){
				return new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,
						new ASTNode(AT_SUM,vector<ASTNode*>{
							new ASTNode(AT_NUMBER,"1"),
							new ASTNode(AT_APPLY,"pow",vector<ASTNode*>{
								new ASTNode(*a1),
								new ASTNode(AT_NUMBER,"2")
							})
						})
					)),
					derivative(a1,var)
				});
			} else if(node->value=="sqrt"){
				return new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,
						new ASTNode(AT_PRODUCT,vector<ASTNode*>{
							new ASTNode(AT_NUMBER,"2"),
							new ASTNode(*a1)
						})
					)),
					derivative(a1,var)
				});
			} else if(node->value=="exp"){
				return new ASTNode(AT_PRODUCT,vector<ASTNode*>{
					new ASTNode(*node),
					derivative(a1,var)
				});
			} else throw ParseError("Cannot compute derivative of function "+node->value);
		}

		default:
			throw TraceException("Unexpected node type "+to_string(node->type)+" in treefunc d()");
	}
}


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
const unordered_map<string,function<ASTNode*(vector<ASTNode*>)>> treefunctions={
	{"d",[](vector<ASTNode*> x){NARGS("d",2);
		if(x[1]->type!=AT_VARIABLE)throw ParseError("Second argument to d() must be a variable");
		return derivative(x[0],x[1]->value);
	}},
};
#undef NARGS
const unordered_map<string,long double> constants={
	{"PI",M_PI},
	{"E",M_E},
	{"PHI",(1+sqrt(5))/2},
};

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
				long double v=strtold(child->value.data(),nullptr);
				if(node->type==AT_SUM)number+=v;
				else number*=v;
				node->children.erase(node->children.begin()+i);
				delete child;
				i--;
			}
		}
		if((node->type==AT_SUM&&number!=0)||(node->type==AT_PRODUCT&&number!=1)){
			node->children.push_back(new ASTNode(AT_NUMBER,convertstring(number)));
		}
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
	if(node->type!=AT_SUM&&node->type!=AT_PRODUCT)return false;
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

bool foldReciprocals(ASTNode *node){
	if(node->type!=AT_PRODUCT)return false;
	int nfound=0;
	for(ASTNode *child : node->children){
		if(child->type==AT_RECIPROCAL){
			nfound++;
			if(nfound==2)break;
		}
	}
	if(nfound<2)return false;
	ASTNode *prod=new ASTNode(AT_PRODUCT);
	ASTNode *recip=new ASTNode(AT_RECIPROCAL,vector<ASTNode*>(1,prod));
	for(size_t i=0;i<node->children.size();i++){
		ASTNode *child=node->children[i];
		if(child->type!=AT_RECIPROCAL)continue;
		prod->children.insert(prod->children.end(),child->children.begin(),child->children.end());
		child->children.clear();
		node->children.erase(node->children.begin()+i);
		delete child;
		i--;
	}
	collapseSums(prod);
	node->children.push_back(recip);
	return true;
}

bool productContains(ASTNode *prod,ASTNode *target){
	if(prod->type!=AT_PRODUCT)throw TraceException("First argument to productContains not a product");
	for(ASTNode *child : prod->children){
		if(*child==*target)return true;
	}
	return false;
}

ASTNode* makeProductWithoutSubject(ASTNode *prod,ASTNode *subject){
	ASTNode *prodcopy=new ASTNode(*prod);
	for(size_t i=0;i<prodcopy->children.size();i++){
		ASTNode *factor=prodcopy->children[i];
		if(*factor!=*subject)continue;
		prodcopy->children.erase(prodcopy->children.begin()+i);
		break;
	}
	return prodcopy;
}

//returns (sum of terms collected as multiplier of subject, terms of the sum that should be deleted
//if this multiplier * subject is added to the expression itself; are in order of sum)
pair<ASTNode*,vector<ASTNode*>> multiplicitySum(ASTNode *sum,ASTNode *subject){
	if(sum->type!=AT_SUM)throw TraceException("First argument to multiplicitySum not a sum");
	//cerr<<"Collecting multiplier for "<<*subject<<" in "<<*sum<<endl;
	ASTNode *result=new ASTNode(AT_SUM);
	vector<ASTNode*> delnodes;
	for(size_t i=0;i<sum->children.size();i++){
		ASTNode *term=sum->children[i];
		if(*term==*subject){
			result->children.push_back(new ASTNode(AT_NUMBER,"1"));
			delnodes.push_back(term);
		} else if(term->type==AT_PRODUCT&&productContains(term,subject)){
			result->children.push_back(makeProductWithoutSubject(term,subject));
			delnodes.push_back(term);
		} else if(term->type==AT_NEGATIVE){
			ASTNode *negnode=term->children[0];
			if(*negnode==*subject){
				result->children.push_back(new ASTNode(AT_NUMBER,"-1"));
				delnodes.push_back(term);
			} else if(negnode->type==AT_PRODUCT&&productContains(negnode,subject)){
				result->children.push_back(
					new ASTNode(AT_NEGATIVE,vector<ASTNode*>(1,makeProductWithoutSubject(negnode,subject)))
				);
				delnodes.push_back(term);
			}
		}
	}
	//cerr<<"Result: "<<*result<<endl;
	//cerr<<"Terms deleted: "<<endl;
	//for(ASTNode *n : delnodes)cerr<<"deleted: "<<*n<<endl;
	return make_pair(result,delnodes);
}

bool foldSimilarTermsSum(ASTNode *node){
	if(node->type!=AT_SUM)return false;
	//cerr<<__func__<<" on node "<<*node<<endl;
	bool changed=false;
	set<string> variables;
	for(size_t i=0;i<node->children.size();i++){
		ASTNode *child=node->children[i];
		if(child->type==AT_VARIABLE)variables.insert(child->value);
		else if(child->type==AT_PRODUCT){
			for(size_t j=0;j<child->children.size();j++){
				ASTNode *child2=child->children[j];
				if(child2->type==AT_VARIABLE)variables.insert(child2->value);
				else if(child->type==AT_NEGATIVE&&child->children[0]->type==AT_VARIABLE){
					variables.insert(child->children[0]->value);
				}
			}
		} else if(child->type==AT_NEGATIVE&&child->children[0]->type==AT_VARIABLE){
			variables.insert(child->children[0]->value);
		}
	}
	for(string varname : variables){
		ASTNode *varnode=new ASTNode(AT_VARIABLE,varname);
		pair<ASTNode*,vector<ASTNode*>> msump=multiplicitySum(node,varnode);
		ASTNode *msum=msump.first;
		vector<ASTNode*> &delnodes=msump.second;
		simplifyTree(msum);
		if(msum->type==AT_SUM|| //should be a single term to be worth simplifying
		   delnodes.size()==1){ //apparently no other nodes folded
			delete varnode;
			continue;
		}
		size_t dnidx=0;
		for(size_t i=0;i<node->children.size()&&dnidx<delnodes.size();i++){
			ASTNode *child=node->children[i];
			if(child==delnodes[dnidx]){
				node->children.erase(node->children.begin()+i);
				delete child;
				i--;
				dnidx++;
			}
		}
		ASTNode *newprod=new ASTNode(AT_PRODUCT,vector<ASTNode*>{msum,varnode});
		simplifyTree(newprod);
		node->children.push_back(newprod);
		changed=true;
	}
	//cerr<<"Result: "<<*node<<endl;
	return changed;
}

bool foldSimilarTerms(ASTNode *node){
	bool changed=false;
	changed=foldSimilarTermsSum(node)||changed;
	//changed=foldSimilarTermsProduct(ASTNode *node)||changed;
	return changed;
}

bool removeTrivialTerms(ASTNode *node){
	if(node->type!=AT_SUM&&node->type!=AT_PRODUCT)return false;
	bool changed=false;
	for(size_t i=0;i<node->children.size();i++){
		ASTNode *child=node->children[i];
		if(child->type!=AT_NUMBER)continue;
		long double v=strtold(child->value.data(),nullptr);
		if((node->type==AT_SUM&&v==0)||
		   (node->type==AT_PRODUCT&&v==1)){
			node->children.erase(node->children.begin()+i);
			delete child;
			i--;
			changed=true;
		} else if(node->type==AT_PRODUCT&&v==0){
			for(ASTNode *child : node->children){
				delete child;
			}
			node->children.clear();
			node->type=AT_NUMBER;
			node->value="0";
			return true;
		} else if(node->type==AT_PRODUCT&&v==-1){
			node->children.erase(node->children.begin()+i);
			delete child;
			ASTNode *newprod=new ASTNode(AT_PRODUCT);
			newprod->children=move(node->children);
			node->type=AT_NEGATIVE;
			node->children.push_back(newprod);
			return true;
		}
	}
	return changed;
}

bool simplifyTree(ASTNode *node){
	bool changed=false;
	//cerr<<"simplifyTree on "<<*node<<endl;
	switch(node->type){
		case AT_SUM:
		case AT_PRODUCT:{
			while(true){
				//cerr<<"Entering loop, expr -> "<<stringifyTree(node)<<endl;
				for(ASTNode *child : node->children){
					changed=simplifyTree(child)||changed;
				}
				if(collapseSums(node)){changed=true; continue;}
				if(foldNumbers(node)){changed=true; continue;}
				if(foldNegatives(node)){ //can change node type
					simplifyTree(node);
					return true;
				}
				if(foldReciprocals(node)){changed=true; continue;}
				if(foldSimilarTerms(node)){changed=true; continue;}
				if(removeTrivialTerms(node)){ //can change node type
					simplifyTree(node);
					return true;
				}
				break;
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
			if(node->children.size()==0){
				//cerr<<"############"<<endl;
				//cerr<<*node<<endl;
				node->value=convertstring((long double)(node->type==AT_SUM?0:1));
				node->type=AT_NUMBER;
				node->children.clear();
				//cerr<<*node<<endl;
				//cerr<<"############"<<endl;
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
			auto it=functions.find(node->value);
			if(it!=functions.end()){
				if(allnumber){
					vector<long double> values;
					values.reserve(node->children.size());
					for(ASTNode *child : node->children){
						values.push_back(strtold(child->value.data(),nullptr));
					}
					long double res=it->second(values);
					node->type=AT_NUMBER;
					for(ASTNode *child : node->children){
						delete child;
					}
					node->children.clear();
					node->value=convertstring(res);
					changed=true;
				}
			} else {
				auto it=treefunctions.find(node->value);
				if(it==treefunctions.end()){
					throw ParseError("Unknown function "+node->value);
				}
				ASTNode *newsubtree=it->second(node->children);
				for(ASTNode *child : node->children){
					delete child;
				}
				node->children=move(newsubtree->children);
				node->type=newsubtree->type;
				node->value=move(newsubtree->value);
				delete newsubtree;
				changed=true;
			}
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
			ss<<'{';
			stringifyTree(node->children[0],ss);
			for(size_t i=1;i<node->children.size();i++){
				ss<<(node->type==AT_SUM?" + ":" \\cdot ");
				stringifyTree(node->children[i],ss);
			}
			ss<<'}';
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
			ss<<"\\frac1{";
			stringifyTree(node->children[0],ss);
			ss<<'}';
			break;

		case AT_APPLY:
			if(node->value=="pow"){
				ss<<'{';
				stringifyTree(node->children[0],ss);
				ss<<"}^{";
				stringifyTree(node->children[1],ss);
				ss<<'}';
			} else if(node->value=="exp"){
				ss<<"e^{";
				stringifyTree(node->children[0],ss);
				ss<<'}';
			} else if(node->value=="sqrt"){
				ss<<"\\sqrt{";
				stringifyTree(node->children[0],ss);
				ss<<'}';
			} else {
				ss<<"\\mathrm{"<<node->value<<"}(";
				if(node->children.size()==0)ss<<')';
				else {
					stringifyTree(node->children[0],ss);
					for(size_t i=1;i<node->children.size();i++){
						ss<<',';
						stringifyTree(node->children[i],ss);
					}
					ss<<')';
				}
			}
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
