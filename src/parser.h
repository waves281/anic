#ifndef _PARSER_H_
#define _PARSER_H_

#include "customOperators.h"
#include "constantDefs.h"

#include "lexer.h"
#include "../var/lexerStruct.h"

// action definitions
#define ACTION_SHIFT 1
#define ACTION_REDUCE 2
#define ACTION_ACCEPT 3
#define ACTION_GOTO 4
#define ACTION_ERROR 5

struct parserNodeStruct {
	int action; // the action to take in this situation (ACTION_ defines above)
	int n; // either the state to go to (SHIFT/GOTO) or the rule to reduce by (REDUCE)
};
typedef struct parserNodeStruct ParserNode;

class Tree {
	private:
		// object-local variables
		Tree *nextInternal;
		Tree *backInternal;
		Tree *childInternal;
		Tree *parentInternal;
	public:
		// allocators/deallocators
		Tree();
		Tree(Tree *next, Tree *back, Tree *child, Tree *parent);
		~Tree();
		// public interface
		Tree &next();
		Tree &back();
		Tree &child();
		Tree &parent();
		// traversal operators
		Tree &operator+(unsigned int n);
		Tree &operator-(unsigned int n);
		Tree &operator*(unsigned int n);
		Tree &operator&(unsigned int n);
		Tree &operator+();
		Tree &operator-();
		Tree &operator*();
		Tree &operator&();
};

Tree *parse(vector<Token> *lexeme, char *fileName);

#endif
