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
		bool toStringHandled; // used for recursion detection in operator string()
		// mutators
		bool delatch();
		bool copyDelatch();
		bool destream();
		bool copyDestream();
		// allocators/deallocators
		virtual ~Type();
		// core methods
		// virtual
		virtual bool isComparable(const Type &otherType) const = 0;
		virtual Type *copy() const = 0;
		// non-virtual
		bool baseEquals(const Type &otherType) const;
		bool baseSendable(const Type &otherType) const;
		// operators
		// virtual
		virtual bool operator==(const Type &otherType) const = 0;
		virtual Type *operator,(Type &otherType) const = 0;
		virtual Type *operator>>(Type &otherType) const = 0;
		virtual operator string() = 0;
		// non-virtual
		operator bool() const;
		bool operator!() const;
		bool operator!=(const Type &otherType) const;
};

// Type subclasses

class TypeList : public Type {
	public:
		// data members
		vector<Type *> list; // pointers to the underlying list of types
		// allocators/deallocators
		TypeList(const vector<Type *> &list);
		TypeList(Type *type);
		TypeList();
		~TypeList();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy() const;
		// operators
		bool operator==(const Type &otherType) const;
		bool operator==(int kind) const;
		Type *operator,(Type &otherType) const;
		Type *operator>>(Type &otherType) const;
		operator string();
};

class ErrorType : public Type {
	public:
		// allocators/deallocators
		ErrorType();
		~ErrorType();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy() const;
		// operators
		bool operator==(const Type &otherType) const;
		bool operator==(int kind) const;
		Type *operator,(Type &otherType) const;
		Type *operator>>(Type &otherType) const;
		operator string();
};

// Type kind specifiers

#define STD_NULL 0
#define STD_STD 1

#define STD_MIN_COMPARABLE 2

#define STD_INT 2
#define STD_FLOAT 3
#define STD_BOOL 4
#define STD_CHAR 5
#define STD_STRING 6

#define STD_MAX_COMPARABLE 6

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
		int kind; // the kind of standard type that this is
		// allocators/deallocators
		StdType(int kind, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~StdType();
		// core methods
		bool isComparable(const Type &otherType) const;
		int kindCompare(const StdType &otherType) const; // returns kind resulting from sending *this to otherType, STD_NULL if the comparison is invalid
		Type *copy() const;
		// operators
		bool operator==(const Type &otherType) const;
		bool operator==(int kind) const;
		Type *operator,(Type &otherType) const;
		Type *operator>>(Type &otherType) const;
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
		bool isComparable(const Type &otherType) const;
		Type *copy() const;
		// operators
		bool operator==(const Type &otherType) const;
		bool operator==(int kind) const;
		Type *operator,(Type &otherType) const;
		Type *operator>>(Type &otherType) const;
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
		ObjectType(const vector<TypeList *> &constructorTypes, int suffix = SUFFIX_CONSTANT, int depth = 0);
		ObjectType(const vector<TypeList *> &constructorTypes, const vector<string> &memberNames, const vector<Type *> &memberTypes, int suffix = SUFFIX_CONSTANT, int depth = 0);
		~ObjectType();
		// core methods
		bool isComparable(const Type &otherType) const;
		Type *copy() const;
		// operators
		bool operator==(const Type &otherType) const;
		bool operator==(int kind) const;
		Type *operator,(Type &otherType) const;
		Type *operator>>(Type &otherType) const;
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
		TypeStatus(Type *type, const TypeStatus &otherStatus);
		~TypeStatus();
		// converters
		operator Type *() const;
		// operators
		TypeStatus &operator=(const TypeStatus &otherStatus);
		TypeStatus &operator=(Type *otherType);
		TypeStatus &operator=(Tree *otherTree);
		Type &operator*() const;
		Type *operator->() const;
		bool operator==(const Type &otherType) const;
		bool operator!=(const Type &otherType) const;
};

// external linkage specifiers
extern Type *nullType;
extern Type *errType;

// post-includes
#include "parser.h"

// Type to string helper blocks

#define TYPE_TO_STRING_HEADER \
	/* if we have reached a recursive loop, return this fact */\
	if (toStringHandled) {\
		return "<recursion>";\
	} else { /* otherwise, set the recursive printing flag */\
		toStringHandled = true;\
	}\
	/* prepare to compute the string normally */\
	string acc

#define TYPE_TO_STRING_FOOTER \
	/* unset the recursive printing flag */\
	toStringHandled = false;\
	/* return the completed accumulator */\
	return acc

#endif
