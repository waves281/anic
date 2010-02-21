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

#define STD_NULL 1
#define STD_NODE 2

#define STD_MIN_COMPARABLE 3 /* anything in the range is considered comparable */

#define STD_INT 3
#define STD_FLOAT 4
#define STD_BOOL 5
#define STD_CHAR 6
#define STD_STRING 7

#define STD_MAX_COMPARABLE 7 /* anything in the range is considered comparable */

#define STD_NOT 8
#define STD_COMPLEMENT 9
#define STD_DPLUS 10
#define STD_DMINUS 11

#define STD_DOR 12
#define STD_DAND 13
#define STD_OR 14
#define STD_XOR 15
#define STD_AND 16
#define STD_DEQUALS 17
#define STD_NEQUALS 18
#define STD_LT 19
#define STD_GT 20
#define STD_LE 21
#define STD_GE 22
#define STD_LS 23
#define STD_RS 24
#define STD_TIMES 25
#define STD_DIVIDE 26
#define STD_MOD 27

#define STD_PLUS 28
#define STD_MINUS 29

// Type suffix specifiers

#define SUFFIX_NONE 0
#define SUFFIX_LATCH -1

class Type {
	public:
		// data members
		int kind; // the class of type that this is
		Tree *base; // the Node that defines this type
		int suffix; // positive values indicate stream depth
		Type *next; // the next part of the type's compounding
		// allocators/deallocators
		Type(int kind);
		Type(int kind, Tree *base);
		Type(int kind, Tree *base, int suffix);
		~Type();
		// operators
		bool operator==(int kind);
		bool operator!=(int kind);
		bool operator<=(int kind);
		bool operator>=(int kind);
};

// main semantic analysis function

int sem(Tree *treeRoot, vector<vector<Tree *> *> &parsemes, SymbolTable *&stRoot, bool verboseOutput, int optimizationLevel, bool eventuallyGiveUp);

#endif
