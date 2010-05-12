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

// core Type class

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
		virtual bool isComparable(Type &otherType) = 0;
		// non-virtual
		bool baseEquals(Type &otherType);
		bool baseSendable(Type &otherType);
		// operators
		// virtual
		virtual bool operator==(Type &otherType) = 0;
		virtual Type *operator,(Type &otherType) = 0;
		virtual Type *operator>>(Type &otherType) = 0;
		virtual operator string() = 0;
		// non-virtual
		operator bool();
		bool operator!();
		bool operator!=(Type &otherType);
};

// Type subclasses

class TypeList : public Type {
	public:
		// data members
		vector<Type *> list; // pointers to the underlying list of types
		// allocators/deallocators
		TypeList(vector<Type *> &list);
		TypeList(Type *type);
		TypeList();
		~TypeList();
		// core methods
		bool isComparable(Type &otherType);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type *operator,(Type &otherType);
		Type *operator>>(Type &otherType);
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		~ErrorType();
		// core methods
		bool isComparable(Type &otherType);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type *operator,(Type &otherType);
		Type *operator>>(Type &otherType);
		operator string();
};

// Type kind specifiers

#define STD_NULL 0
#define STD_NODE 1
#define STD_STD 2

#define STD_MIN_COMPARABLE 3

#define STD_INT 3
#define STD_FLOAT 4
#define STD_BOOL 5
#define STD_CHAR 6
#define STD_STRING 7

#define STD_MAX_COMPARABLE 7

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

class StdType : public Type {
	public:
		// data members
		int kind; // the kind of standard type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// core methods
		bool isComparable(Type &otherType);
		int kindCompare(StdType &otherType); // returns kind resulting from sending *this to otherType, STD_NULL if the comparison is invalid
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type *operator,(Type &otherType);
		Type *operator>>(Type &otherType);
		operator string();
};

class FilterType : public Type {
	public:
		// data members
		TypeList *from; // the source of this object type
		TypeList *to; // the destination of this object type
		// allocators/deallocators
		FilterType(Type *from = NULL, Type *to = NULL, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~FilterType();
		// core methods
		bool isComparable(Type &otherType);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type *operator,(Type &otherType);
		Type *operator>>(Type &otherType);
		operator string();
};

class ObjectType : public Type {
	public:
		// data members
		vector<TypeList *> constructorTypes; // list of the types of this object's constructors (each one is a TypeList)
		vector<string> memberNames; // list of names of raw non-constructor members of this object
		vector<Type *> memberTypes; // list of types of raw non-constructor members of this object
		// allocators/deallocators
		ObjectType(int suffix = SUFFIX_CONSTANT, int depth = 0);
		ObjectType(vector<TypeList *> &constructorTypes, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// core methods
		bool isComparable(Type &otherType);
		// operators
		bool operator==(Type &otherType);
		bool operator==(int kind);
		Type *operator,(Type &otherType);
		Type *operator>>(Type &otherType);
		operator string();
};

// typing status block

class TypeStatus {
	public:
		// data members
		Type *type; // the encapsulated type
		Tree *recall; // the encapsulated recall binding
		Type *retType; // the return type for the current node
		// allocators/deallocators
		TypeStatus(Type *type = NULL, Tree *recall = NULL, Type *retType = NULL);
		~TypeStatus();
		// converters
		operator Type *();
		operator Tree *();
		operator bool(); // tests for NULLity of type, not for whether it's the error type or not!
		// operators
		TypeStatus &operator=(TypeStatus otherStatus);
		TypeStatus &operator=(Type *otherType);
		TypeStatus &operator=(Tree *otherTree);
		Type &operator*();
		Type *operator->();
		bool operator==(Type &otherType);
		bool operator!=(Type &otherType);
};

// external linkage specifiers
extern Type *nullType;
extern Type *errType;

// post-includes
#include "parser.h"

#endif