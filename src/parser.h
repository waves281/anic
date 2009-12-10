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
	unsigned int n; // either the state to go to (SHIFT/GOTO) or the rule to reduce by (REDUCE)
};
typedef struct parserNodeStruct ParserNode;

class Tree {
	private:
		// object-local variables
		Token tInternal;
		Tree *nextInternal;
		Tree *backInternal;
		Tree *childInternal;
		Tree *parentInternal;
	public:
		// allocators/deallocators
		Tree();
		Tree(Token &t, Tree *next, Tree *back, Tree *child, Tree *parent);
		~Tree();
		// accessors
		Token &t();
		// traversal operators
		Tree *operator+(unsigned int n);
		Tree *operator-(unsigned int n);
		Tree *operator*(unsigned int n);
		Tree *operator&(unsigned int n);
		Tree *operator+();
		Tree *operator-();
		Tree *operator*();
		Tree *operator&();
		// binary concatenators
		Tree &operator+=(Tree *&next);
		Tree &operator*=(Tree *&child);
		Tree &operator&=(Tree *&parent);
};

Tree *parse(vector<Token> *lexeme, char *fileName);

#endif
