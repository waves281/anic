#ifndef _PARSER_H_
#define _PARSER_H_

#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"

#include "lexer.h"

// forward declarations
class SymbolTable;
class Type;

// action definitions
#define ACTION_SHIFT 1
#define ACTION_REDUCE 2
#define ACTION_ACCEPT 3
#define ACTION_GOTO 4
#define ACTION_ERROR 5

struct parserNodeStruct {
	int action; // the action to take in this situation (ACTION_ defines above)
	unsigned int n; // either the state to go to (SHIFT/GOTO) or the rule to reduce by (REDUCE)
};
typedef struct parserNodeStruct ParserNode;

class Tree {
	public:
		// object-local variables
		Token t;
		Tree *next;
		Tree *back;
		Tree *child;
		Tree *parent;
		SymbolTable *env; // the symbol environment in which this node occurs
		Type *type; // the Type coming OUT of this node; NULL if it hasn't been traced or is unresolvable
		// allocators/deallocators
		Tree();
		Tree(Token &t);
		Tree(Token &t, Tree *next, Tree *back, Tree *child, Tree *parent);
		~Tree();
		// traversal operators
		Tree *goNext(unsigned int n);
		Tree *goBack(unsigned int n);
		Tree *goChild(unsigned int n);
		Tree *goParent(unsigned int n);
		// binary attatchers
		void operator+=(Tree *next);
		void operator-=(Tree *back);
		void operator*=(Tree *child);
		void operator&=(Tree *parent);
		// generalized traverser
		Tree *operator()(char *s);
};

string id2String(Tree *t);
string idTip(string &qi);
string idEnd(string &qi);
vector<string> qiChop(string &qi);
Tree *parse(vector<Token> *lexeme, const char *fileName, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

// post-includes

#include "../tmp/parserStruct.h"
#include "semmer.h"

#endif
