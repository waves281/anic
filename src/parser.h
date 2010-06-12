#ifndef _PARSER_H_
#define _PARSER_H_

#include "globalDefs.h"
#include "constantDefs.h"
#include "driver.h"

#include "lexer.h"
#include "../tmp/parserStruct.h"
#include "types.h"

// forward declarations
class SymbolTable;
class Type;
class TypeStatus;
class IRTree;

class Tree {
	public:
		// data members
		Token t;
		Tree *next;
		Tree *back;
		Tree *child;
		Tree *parent;
		SymbolTable *env; // the symbol environment in which this node occurs
		TypeStatus status; // the status coming OUT of this node
		// allocators/deallocators
		Tree(const Token &t);
		Tree(const Token &t, Tree *next, Tree *back, Tree *child, Tree *parent);
		Tree(const TypeStatus &status);
		~Tree();
		// core methods
		IRTree *code() const;
		// comparison operators
		bool operator==(int tokenType) const;
		bool operator!=(int tokenType) const;
		// traversal operators
		Tree *goNext(unsigned int n) const;
		Tree *goBack(unsigned int n) const;
		Tree *goChild(unsigned int n) const;
		Tree *goParent(unsigned int n) const;
		Tree *bottom() const;
		// binary attatchers
		void operator+=(Tree *next);
		void operator-=(Tree *back);
		void operator*=(Tree *child);
		void operator&=(Tree *parent);
		// converters
		operator string() const; // only for SuffixedIdentifier or NonArraySuffixedIdentifier kinds
};

int parse(vector<Token> *lexeme, Tree *&parseme, string &fileName);

// post-includes
#include "semmer.h"
#include "genner.h"

#endif
