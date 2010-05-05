#ifndef _TYPES_H_
#define _TYPES_H_

#include "mainDefs.h"
#include "system.h"
#include "constantDefs.h"

#include "../tmp/parserStruct.h"

// forward declarations
class Tree;
class SymbolTable;

// Type category specifiers
#define CATEGORY_TYPELIST 0
#define CATEGORY_STDTYPE 1
#define CATEGORY_FILTERTYPE 2
#define CATEGORY_OBJECTTYPE 3
#define CATEGORY_ERRORTYPE 4

// Type suffix specifiers
#define SUFFIX_CONSTANT 0
#define SUFFIX_LATCH 1
#define SUFFIX_STREAM 2
#define SUFFIX_ARRAY 3

// forward declarations
class Type;
class TypeList;
class MemberedType;
class ErrorType;
class StdType;
class FilterType;
class ObjectType;
class ErrorType;
class TypeStatus;

class Type {
	public:
		// data members
		int category; // the category that this type belongs to
		int suffix; // the type suffix (constant, latch, stream, or array)
		int depth; // stream depth of arrays and streams
		// mutators
		void delatch();
		// allocators/deallocators
		virtual ~Type();
		// core methods
		// virtual
		virtual bool isComparable() = 0;
		// non-virtual
		bool baseEquals(Type &otherType);
		bool baseSendable(Type &otherType);
		// operators
		// virtual
		virtual bool operator==(Type &otherType) = 0;
		virtual Type &operator>>(Type &otherType) = 0;
		virtual operator string() = 0;
		// non-virtual
		operator bool();
		bool operator!();
		bool operator!=(Type &otherType);
};

class TypeList : public Type {
	public:
		// data members
		vector<Type *> list; // pointers to the underlying list of types
		// allocators/deallocators
		TypeList(Tree *tree, Tree *&recall);
		TypeList(Type *type);
		TypeList();
		~TypeList();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

// Type kind specifiers

#define STD_NULL 0

#define STD_MIN_COMPARABLE 1 /* anything in the range is considered comparable */

#define STD_INT 1
#define STD_FLOAT 2
#define STD_BOOL 3
#define STD_CHAR 4
#define STD_STRING 5

#define STD_MAX_COMPARABLE 5 /* anything in the range is considered comparable */

#define STD_NOT 6
#define STD_COMPLEMENT 7
#define STD_DPLUS 8
#define STD_DMINUS 9

#define STD_DOR 10
#define STD_DAND 11
#define STD_OR 12
#define STD_XOR 13
#define STD_AND 14
#define STD_DEQUALS 15
#define STD_NEQUALS 16
#define STD_LT 17
#define STD_GT 18
#define STD_LE 19
#define STD_GE 20
#define STD_LS 21
#define STD_RS 22
#define STD_TIMES 23
#define STD_DIVIDE 24
#define STD_MOD 25

#define STD_PLUS 26
#define STD_MINUS 27

class StdType : public Type {
	public:
		// data members
		int kind; // the class of type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

class FilterType : public Type {
	public:
		// data members
		TypeList *from; // the source of this object type
		TypeList *to; // the destination of this object type
		// allocators/deallocators
		FilterType(Type *from, Type *to, int suffix = SUFFIX_CONSTANT, int depth = 0);
		FilterType(Type *from, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

class ObjectType : public Type {
	public:
		// data members
		SymbolTable *base; // the Node that defines this type (base->defSite is Declaration->TypedStaticTerm->Node->Object)
		vector<TypeList *> constructorTypes; // list of the types of this object's constructors (each one is a TypeList)
		vector<string> memberNames; // list of names of raw non-constructor members of this object
		vector<Type *> memberTypes; // list of types of raw non-constructor members of this object
		// allocators/deallocators
		ObjectType(SymbolTable *base, Tree *&recall, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// core methods
		bool isComparable();
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type &operator>>(Type &otherType);
		operator string();
};

// typing status block

class TypeStatus {
	public:
		// data members
		Type *type; // the encapsulated type
		Tree *recall; // the encapsulated recall binding
		// allocators/deallocators
		TypeStatus();
		TypeStatus(Type *type, Tree *recall = NULL);
		~TypeStatus();
		// converters
		operator Type *();
		operator Tree *();
		operator bool(); // tests for NULLity of type, not for whether it's the error type or not!
		// operators
		TypeStatus &operator=(TypeStatus &otherStatus);
		TypeStatus &operator=(Type *otherType);
		Type &operator*();
		bool operator==(Type &otherType);
		bool operator!=(Type &otherType);
};

// external linkage specifiers
extern Type *nullType;
extern Type *errType;

// post-includes
#include "parser.h"

#endif