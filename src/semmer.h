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

// Type suffix specifiers

#define SUFFIX_CONSTANT 0
#define SUFFIX_LATCH 1
#define SUFFIX_STREAM 2
#define SUFFIX_ARRAY 3
#define SUFFIX_STREAMARRAY 4

// forward declarations
class StdType;
class FilterType;
class ObjectType;

class Type {
	public:
		// data members
		int suffix; // the type suffix
		int depth; // stream depth of arrays and streams
		// mutators
		void delatch();
		// operators
		// virtual
		virtual bool operator==(StdType &otherType) = 0;
		virtual bool operator==(FilterType &otherType) = 0;
		virtual bool operator==(ObjectType &otherType) = 0;
		virtual bool operator>>(StdType &otherType) = 0;
		virtual bool operator>>(FilterType &otherType) = 0;
		virtual bool operator>>(ObjectType &otherType) = 0;
		virtual bool isErr() = 0;
		// non-vitrual
		bool operator!=(Type &otherType);
};

class MemberedType : public Type {
	public:
		// data members
		vector<Type *> memberList;
};

class ErrorType : public Type {
	public:
		// operators
		bool isErr();
};

// Type kind specifiers

#define STD_NULL 0
#define STD_NODE 1

#define STD_MIN_COMPARABLE 2 /* anything in the range is considered comparable */

#define STD_INT 2
#define STD_FLOAT 3
#define STD_BOOL 4
#define STD_CHAR 5
#define STD_STRING 6

#define STD_MAX_COMPARABLE 6 /* anything in the range is considered comparable */

#define STD_NOT 7
#define STD_COMPLEMENT 8
#define STD_DPLUS 9
#define STD_DMINUS 10

#define STD_DOR 11
#define STD_DAND 12
#define STD_OR 13
#define STD_XOR 14
#define STD_AND 15
#define STD_DEQUALS 16
#define STD_NEQUALS 17
#define STD_LT 18
#define STD_GT 19
#define STD_LE 20
#define STD_GE 21
#define STD_LS 22
#define STD_RS 23
#define STD_TIMES 24
#define STD_DIVIDE 25
#define STD_MOD 26

#define STD_PLUS 27
#define STD_MINUS 28

class StdType : public Type {
	public:
		// data members
		int kind; // the class of type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// operators
		bool operator==(StdType &otherType);
		bool operator==(FilterType &otherType);
		bool operator==(ObjectType &otherType);
		bool operator>>(StdType &otherType);
		bool operator>>(FilterType &otherType);
		bool operator>>(ObjectType &otherType);
};

class FilterType : public MemberedType {
	public:
		// data members
		Type *from; // the source of this object type
		Type *to; // the destination of this object type
		// allocators/deallocators
		FilterType(Type *from, Type *to, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// operators
		bool operator==(StdType &otherType);
		bool operator==(FilterType &otherType);
		bool operator==(ObjectType &otherType);
		bool operator>>(StdType &otherType);
		bool operator>>(FilterType &otherType);
		bool operator>>(ObjectType &otherType);
};

class ObjectType : public MemberedType {
	public:
		// data members
		SymbolTable *base; // the Node that defines this type
		vector<Type *> constructorList; // list of this object's constructors
		// allocators/deallocators
		ObjectType(SymbolTable *base, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// operators
		bool operator==(StdType &otherType);
		bool operator==(FilterType &otherType);
		bool operator==(ObjectType &otherType);
		bool operator>>(StdType &otherType);
		bool operator>>(FilterType &otherType);
		bool operator>>(ObjectType &otherType);
};

// semantic analysis helper blocks

#define GET_TYPE_HEADER \
	/* if the type is memoized, short-circuit evaluate */\
	if (tree->type != NULL) {\
		return tree->type;\
	}\
	/* otherwise, compute the type normally */\
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
