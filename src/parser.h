#ifndef _PARSER_H_
#define _PARSER_H_

#include "constantDefs.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

// forward declaration
class SymbolTable;

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
		Tree &operator+=(Tree *&next);
		Tree &operator-=(Tree *&back);
		Tree &operator*=(Tree *&child);
		Tree &operator&=(Tree *&parent);
		void operator+=(int x);
		void operator-=(int x);
		void operator*=(int x);
		void operator&=(int x);
		// generalized traverser
		Tree *operator()(char *s);
};

Tree *parse(vector<Token> *lexeme, SymbolTable *&stRoot, char *fileName, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

#endif
