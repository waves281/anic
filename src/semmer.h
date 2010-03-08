#ifndef _SEMMER_H_
#define _SEMMER_H_

#include "mainDefs.h"
#include "system.h"
#include "constantDefs.h"

#include "lexer.h"
#include "parser.h"

// SymbolTable node kinds

#define KIND_BLOCK 1
#define KIND_STD 2
#define KIND_IMPORT 3
#define KIND_STATIC_DECL 4
#define KIND_THROUGH_DECL 5
#define KIND_PARAM 6

class SymbolTable {
	public:
		// data members
		int kind;
		string id;
		Tree *defSite; // where the symbol is defined in the Tree (Declaration or Param); NULL for root block and standard nodes
		SymbolTable *parent;
		vector<SymbolTable *> children;
		// allocators/deallocators
		SymbolTable(int kind, string id, Tree *defSite);
		SymbolTable(SymbolTable &st);
		~SymbolTable();
		// deep-copy assignment operator
		SymbolTable &operator=(SymbolTable &st);
		// concatenators
		SymbolTable &operator*=(SymbolTable *st);
};

// Type kind specifiers

#define USR 0

#define TYPE_ERROR 1

#define STD_NULL 2
#define STD_NODE 3

#define STD_MIN_COMPARABLE 4 /* anything in the range is considered comparable */

#define STD_INT 4
#define STD_FLOAT 5
#define STD_BOOL 6
#define STD_CHAR 7
#define STD_STRING 8

#define STD_MAX_COMPARABLE 8 /* anything in the range is considered comparable */

#define STD_NOT 9
#define STD_COMPLEMENT 10
#define STD_DPLUS 11
#define STD_DMINUS 12

#define STD_DOR 13
#define STD_DAND 14
#define STD_OR 15
#define STD_XOR 16
#define STD_AND 17
#define STD_DEQUALS 18
#define STD_NEQUALS 19
#define STD_LT 20
#define STD_GT 21
#define STD_LE 22
#define STD_GE 23
#define STD_LS 24
#define STD_RS 25
#define STD_TIMES 26
#define STD_DIVIDE 27
#define STD_MOD 28

#define STD_PLUS 29
#define STD_MINUS 30

// Type suffix specifiers

#define SUFFIX_NONE 0
#define SUFFIX_LATCH -1

class Type {
	public:
		// data members
		int kind; // the class of type that this is
		Tree *base; // the Node that defines this type
		int suffix; // positive values indicate stream depth, -1 indicates latch
		Type *next; // the next part of the type's compounding
		Type *from; // the source of this object type
		Type *to; // the destination of this object type
		// allocators/deallocators
		Type(int kind);
		Type(int kind, Tree *base);
		Type(int kind, Tree *base, int suffix);
		Type(Type *from, Type *to);
		~Type();
		// operators
		bool operator==(int kind);
		bool operator!=(int kind);
		bool operator<=(int kind);
		bool operator>=(int kind);
};

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->type != NULL) {\
		return tree->type;\
	}\
	/* otherwise, copute the type normally */\
	Type *type = NULL

#define GET_TYPE_FOOTER \
	/* if we could't resolve a valid type, use the error type */\
	if (type == NULL) {\
		type = errType;\
	}\
	/* latch the type to the tree node */\
	tree->type = type;\
	/* return the derived type */\
	return type

// main semantic analysis function

int sem(Tree *treeRoot, vector<Tree *> *parseme, SymbolTable *&stRoot);

#endif
