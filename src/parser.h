#ifndef _PARSER_H_
#define _PARSER_H_

#include "mainDefs.h"
#include "constantDefs.h"
#include "system.h"

#include "lexer.h"
#include "../tmp/parserStruct.h"

// forward declarations
class SymbolTable;
class Type;

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
		// comparison operators
		bool operator==(int tokenType);
		bool operator!=(int tokenType);
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
string idHead(string &id);
string idTail(string &id);
string idEnd(string &id);
int parse(vector<Token> *lexeme, vector<Tree *> *parseme, string &fileName);

// post-includes

#include "semmer.h"

#endif
